#include "ikfast_wrapper.h"

#define isnan std::isnan
#define IKFAST_NO_MAIN // Don't include main() from IKFast

#include IKFAST_SOLVER_CPP


using namespace IKFAST_NAMESPACE;


void queryIKService(shared_ptr<MachineState> ms, int * thisResult, baxter_core_msgs::SolvePositionIK * thisRequest);

namespace MY_NAMESPACE {

eePose ikfast_computeFK(shared_ptr<MachineState> ms, vector<double> joint_angles);
int ikfast_solve(shared_ptr <MachineState> ms, geometry_msgs::Pose pose, double free, IkSolutionListBase<IkReal> &solutions);
bool ikfast_search(shared_ptr <MachineState> ms, geometry_msgs::Pose pose, double free, std::vector<double>& solutions);

bool ikfast_obeys_limits(vector<double> joints, vector<double> joint_min_vector, vector<double> joint_max_vector, double tolerance);
double ikfast_joint_error(vector<double> p1, vector<double> p2);
void ikfast_getSolution(const IkSolutionList<IkReal> &solutions, int i, std::vector<double>& solution);

/**
 * Computes forward kinematics using IK fast and returns an eePose in base. 
 */
eePose ikfast_computeFK(shared_ptr<MachineState> ms, vector<double> joint_angles) {
  bool valid = true;

  IkReal eerot[9],eetrans[3];
  IkReal angles[joint_angles.size()];
  for (unsigned int i = 0; i < joint_angles.size(); i++) {
    angles[i] = joint_angles[i];
  }
  ComputeFk(angles,eetrans,eerot);


  geometry_msgs::PoseStamped pose;
  pose.pose.position.x = eetrans[0];
  pose.pose.position.y = eetrans[1];
  pose.pose.position.z = eetrans[2];
  pose.pose.orientation = tf::createQuaternionMsgFromRollPitchYaw(eerot[0], eerot[1], eerot[2]);

  pose.header.stamp = ros::Time(0);
  pose.header.frame_id =  ms->config.left_or_right_arm + "_arm_mount";
  
  geometry_msgs::PoseStamped transformed_pose;

  ms->config.tfListener->transformPose("base", pose, transformed_pose);
  
  return eePose::fromGeometryMsgPose(transformed_pose.pose);

}


int ikfast_solve(shared_ptr <MachineState> ms, geometry_msgs::Pose pose, double free, IkSolutionList<IkReal> &solutions) {

  KDL::Frame pose_frame;
  tf::poseMsgToKDL(pose, pose_frame);
  
  std::vector<double> vfree(1);
  vfree[0] = free;
  
  // For **Transform6D**, eerot is 9 values for the 3x3 rotation matrix.
  
  
  KDL::Rotation mult;
  KDL::Vector direction;
  
  double trans[3];
  trans[0] = pose_frame.p[0];//-.18;
  trans[1] = pose_frame.p[1];
  trans[2] = pose_frame.p[2];
  
  //cout << "Trans: " << trans[0] << ", " << trans[1] << ", " << trans[2] << endl;
  
  mult = pose_frame.M;
  
  double vals[9];
  vals[0] = mult(0,0);
  vals[1] = mult(0,1);
  vals[2] = mult(0,2);
  vals[3] = mult(1,0);
  vals[4] = mult(1,1);
  vals[5] = mult(1,2);
  vals[6] = mult(2,0);
  vals[7] = mult(2,1);
  vals[8] = mult(2,2);
  
  // IKFast56/61
  ComputeIk(trans, vals, vfree.size() > 0 ? &vfree[0] : NULL, solutions);
  //cout <<  "Ik fast solution: " << solutions.GetNumSolutions() << endl;
  
  
  return solutions.GetNumSolutions();
}

bool ikfast_search(shared_ptr <MachineState> ms, geometry_msgs::Pose pose, double free, std::vector<double>& outsol)  {
  ROS_DEBUG_STREAM_NAMED("ikfast","searchPositionIK");

  
  double vfree = free;

  vector<double> current_joints;
  for (int i = 0; i < NUM_JOINTS; i++) {
    current_joints.push_back(ms->config.trueJointPositions[i]);
  }
  vector<double> joint_min_vector;
  vector<double> joint_max_vector;

  for (int i = 0; i < NUM_JOINTS; i++) {
    joint_min_vector.push_back(ms->config.joint_min[i]);
    joint_max_vector.push_back(ms->config.joint_max[i]);
  }


  double joint_safety = 0.01;

  int counter = 0;
  int num = GetNumFreeParameters();
  int * free_params = GetFreeParameters();
  assert(num == 1);
  int free_joint_idx = free_params[0];
  double step = (joint_max_vector[free_joint_idx] - joint_min_vector[free_joint_idx]) / 100;


  double max_dist = VERYBIGNUMBER;
  int max_idx = -1;
  vector<double> max_solution;


  for (double f = joint_min_vector[free_joint_idx]; f < joint_max_vector[free_joint_idx]; f+= step) {
    IkSolutionList<IkReal> solutions;
    int numsol = ikfast_solve(ms, pose, f, solutions);
    //ROS_INFO_STREAM_NAMED("ikfast","Found " << numsol << " solutions from IKFast");
    vector<double> solution;

    for(int s = 0; s < numsol; ++s) {
      ikfast_getSolution(solutions,s,solution);
      
      if (ikfast_obeys_limits(solution, joint_min_vector, joint_max_vector, joint_safety)) {
        double error = ikfast_joint_error(solution, current_joints);
        if (error < max_dist) {
          max_dist = error;
          max_idx = s;
          max_solution = solution;
        }
      }
    }
  }

  if (max_idx == -1) {
    ROS_ERROR("Returning zero in ein ik fast, falling out of the while loop.");
    return false; 
  } else {
    outsol = max_solution;
    return true;
  }
}

void ikfast_getSolution(const IkSolutionList<IkReal> &solutions, int i, std::vector<double>& solution) {
  solution.clear();
  solution.resize(NUM_JOINTS);

  // IKFast56/61
  const IkSolutionBase<IkReal>& sol = solutions.GetSolution(i);
  std::vector<IkReal> vsolfree( sol.GetFree().size() );
  sol.GetSolution(&solution[0],vsolfree.size()>0?&vsolfree[0]:NULL);
}




void queryIKFast(shared_ptr<MachineState> ms, int * thisResult, baxter_core_msgs::SolvePositionIK * thisRequest) {

  string transform = ms->config.left_or_right_arm + "_arm_mount";
  int num = GetNumFreeParameters();
  int * free_params = GetFreeParameters();
  assert(num == 1);
  int free_joint_idx = free_params[0];
  double free = ms->config.trueJointPositions[free_joint_idx];

  geometry_msgs::PoseStamped base_pose = thisRequest->request.pose_stamp[0];
  base_pose.header.stamp = ros::Time(0);
  geometry_msgs::PoseStamped transformed_pose;
  geometry_msgs::PoseStamped gripper_base_pose;

  ms->config.tfListener->transformPose(transform, base_pose, transformed_pose);
  transformed_pose.header.stamp = ros::Time(0);
  ms->config.tfListener->transformPose(ms->config.left_or_right_arm + "_gripper_base", transformed_pose, gripper_base_pose);
  gripper_base_pose.pose.position.z += 0.005;
  gripper_base_pose.header.stamp = ros::Time(0);
  ms->config.tfListener->transformPose(ms->config.left_or_right_arm + "_arm_mount", gripper_base_pose, transformed_pose);
  
  vector<double> solution;
  bool result = ikfast_search(ms, transformed_pose.pose, free, solution);
  thisRequest->response.isValid.resize(1);

  if (result) {
    thisRequest->response.isValid[0] = 1;
    thisRequest->response.joints.resize(1);
    thisRequest->response.joints[0].position.resize(NUM_JOINTS);
    for (int j = 0; j < NUM_JOINTS; j++) {
      thisRequest->response.joints[0].position[j] = solution[j];
    }
    *thisResult = 1;
  } else {
    *thisResult = 0;
    thisRequest->response.isValid[0] = 0;
  }

  //IkSolutionList<IkReal> solutions;
  //ikfast_solve(ms, transformed_pose.pose, free, solutions);
  

  // for (int i = 0; i < solutions.GetNumSolutions(); i++) {
  //   vector<double> solution;
  //   solution.clear();
  //   solution.resize(GetNumJoints());

  //   const IkSolutionBase<IkReal>& sol = solutions.GetSolution(i);
  //   std::vector<IkReal> vsolfree( sol.GetFree().size() );
  //   sol.GetSolution(&solution[0],vsolfree.size()>0?&vsolfree[0]:NULL);
  //   cout << "ikfast: " ;
  //   for (int j = 0; j < solution.size(); j++) {
  //     cout << solution[j] << ", " ;
  //   }
  //   cout << endl;
  //   for (int j = 0; j < NUM_JOINTS; j++) {
  //     //thisRequest->response.joints[0].position[j] = solution[j];
  //   }

  // }


}






bool ikfast_obeys_limits(vector<double> joints, vector<double> joint_min_vector, vector<double> joint_max_vector, double tolerance) {
  for(unsigned int i = 0; i < joints.size(); i++) {
    if (joints[i] < joint_min_vector[i] + tolerance || joints[i] > joint_max_vector[i] - tolerance) {
      return false;
    }
  }
  return true;

}

double ikfast_joint_error(vector<double> p1, vector<double> p2) {
  double error = 0;
  assert(p1.size() == p2.size());
  for (int i = 0; i < p1.size(); i++) {
    error += fabs(p1[i] - p2[i]);
  }
  return error;
}



void queryIKFastDebug(shared_ptr<MachineState> ms, int * thisResult, baxter_core_msgs::SolvePositionIK * thisRequest) {      
  queryIKService(ms, thisResult, thisRequest);
  
  cout << "computing ikfast" << endl;
  if (thisRequest->response.isValid[0]) {
    vector<double> service_angles(NUM_JOINTS);
    for (int i = 0; i < service_angles.size(); i++) {
      service_angles[i] = thisRequest->response.joints[0].position[i];
    }
    
    eePose fk_service = ikfast_computeFK(ms, service_angles);
    
    queryIKFast(ms, thisResult, thisRequest);
    vector<double> ikfast_angles(NUM_JOINTS);
    for (int i = 0; i < ikfast_angles.size(); i++) {
      ikfast_angles[i] = thisRequest->response.joints[0].position[i];
        }
    
    
    eePose fk_ikfast = ikfast_computeFK(ms, ikfast_angles);
    
    eePose requested = eePose::fromGeometryMsgPose(thisRequest->request.pose_stamp[0].pose);
    
    cout << "requested pose: " << requested << endl;
    cout << "service pose: " << fk_service << endl;
    cout << "ikfast pose: " << fk_ikfast << endl;
    cout << "error (service, ikfast): " <<   eePose::distance(fk_service, fk_ikfast) << endl;
    cout << "error (service, requested): " <<   eePose::distance(fk_service, requested) << endl;
    cout << "error (ikfast, requested): " <<   eePose::distance(fk_ikfast, requested) << endl;
    cout << "error (ikfast, true): " <<   eePose::distance(fk_ikfast, ms->config.trueEEPoseEEPose) << endl;
    
    
  }
  
}
}