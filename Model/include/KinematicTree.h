    ///////////////////////////////////////////////////////////////////////////////////////////////////
   //                                                                                               //
  //                      A class representing a rigid, multibody system                           //
 //                                                                                               //
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef KINEMATICTREE_H_
#define KINEMATICTREE_H_

#include <Branch.h>

class KinematicTree
{
	public:
		// Constructor(s)
		KinematicTree() {}                                                                  // Empty constructor
		
		KinematicTree(const std::vector<Branch> &branches);                                 // Constructor from branches
		
		// Functions
		
		bool update_state(const Eigen::VectorXf &jointPosition,
		                  const Eigen::VectorXf &jointVelocity,
		                  const Pose &basePose,
		                  const Eigen::Matrix<float,6,1> baseTwist);   
		
		Eigen::MatrixXf jacobian(const Eigen::Vector3f &point,
		                         const std::string &branchName);
		
		Eigen::MatrixXf jacobian(const Eigen::Vector3f &point,
		                         const unsigned int &branchNumber);
		
		Eigen::MatrixXf time_derivative(const Eigen::MatrixXf &jacobian);
		
		Eigen::MatrixXf partial_derivative(const Eigen::MatrixXf &jacobian,
		                                   const unsigned int    &jointNumber);
		
		Eigen::MatrixXf inertia()  const {return this->Mjj;}
		Eigen::MatrixXf coriolis() const {return this->Cjj;}
		Eigen::MatrixXf damping()  const {return this->D;}
		Eigen::VectorXf gravity()  const {return this->g;}
		Eigen::VectorXf nonlinear_terms() const {return (this->Cjj+this->D)*this->qdot + this->g;}
		
		// Properties
		
		std::vector<Branch> branch;
		
	private:
	
		unsigned int numJoints;
	
		// Kinematic properties
		Eigen::VectorXf q, qdot;                                                            // Joint position and velocity
		
		// Dynamic properties
		Eigen::MatrixXf Mjj;                                                                // Joint inertia matrix
		Eigen::Matrix3f Mjb;                                                                // Joint/base inertia matrix
		Eigen::MatrixXf Cjj;                                                                // Joint Coriolis matrix
		Eigen::Matrix3f Cjb;                                                                // Joint/base Coriolis matrix
		Eigen::MatrixXf D;                                                                  // Joint damping matrix
		Eigen::VectorXf g;                                                                  // Gravitational torque vector
};                                                                                                  // Semicolon needed after class declarations

#endif
