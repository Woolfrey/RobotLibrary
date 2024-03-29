#include <CartesianTrajectory.h>							// Custom trajectory class
#include <MultiPointTrajectory.h>							// Custom trajectory class
#include <SerialLink.h>								// Serial link robot class

class SerialKinCtrl
{
	public:
		SerialKinCtrl(const SerialLink &serial);				// Constructor
		
		// Set Functions
		bool set_joint_target(Eigen::VectorXf &target);			// Set target for joint trajectory
		bool set_joint_targets(const std::vector<Eigen::VectorXf> &targets, const std::vector<float> &times);
		bool set_feedback_gain(const float &gain);				// Set proportional gain for joint control
		bool set_target_pose(Eigen::Isometry3f &target, float &time);	// Set target pose for Cartesian trajectory
		bool set_target_poses(const std::vector<Eigen::Isometry3f> &targets, const std::vector<float> &times);
		
		// Get Functions
		Eigen::VectorXf get_cartesian_control(const float &time);		// Control to track Cartesian trajectory
		Eigen::MatrixXf get_inverse(const Eigen::MatrixXf &A);		// Get the inverse of the given matrix
		Eigen::MatrixXf get_inverse(const Eigen::MatrixXf &A, const Eigen::MatrixXf &W); // Get the weighted inverse of a given matrix
		Eigen::VectorXf get_joint_control(const float &time);		// Control to track joint trajectory
		Eigen::VectorXf get_pose_error(const Eigen::Isometry3f &desired, const Eigen::Isometry3f &actual);
	
	protected: // SerialDynCtrl can access these

		CartesianTrajectory cartesianTrajectory;				// As it says
		int n;									// Number of joints in the robot
		SerialLink robot;							// Use pointer to memory address?
		std::vector<std::array<float,2>> pLim;				// Array of position limits
		std::vector<float> vLim;						// Array of velocity limits
		MultiPointTrajectory jointTrajectory;					// Joint trajectory object		
	
	private:
		float k = 1.0;								// Proportional gain on pose error

		// Functions
		bool scale_velocity_vector(Eigen::VectorXf &vec, const Eigen::VectorXf ref);			// Ensure feasibility with RMRC
		Eigen::MatrixXf get_joint_limit_weighting();							// For redundant manipulators
		Eigen::VectorXf optimise_manipulability(const float &scalar, const Eigen::MatrixXf &J);	// Redundant task
	
};											// Semicolon needed after class declaration

/******************** Create a controller for the given SerialLink object ********************/
SerialKinCtrl::SerialKinCtrl(const SerialLink &serial)
		: robot(serial)							// Assign the serial link object
		, n(robot.get_number_of_joints())					// Number of joints
		, pLim(robot.get_position_limits())					// Not sure if this is the best way to...
		, vLim(robot.get_velocity_limits())					// ... work with the joint limits
{
	// Worker bees can leave.
	// Even drones can fly away.
	// The Queen is their slave
}

/******************** Set a desired joint configuration to move to ********************/
bool SerialKinCtrl::set_joint_target(Eigen::VectorXf &target)
{
	if(target.size() != this->n)
	{
		std::cerr << "[ERROR] [SERIALKINCTRL] set_joint_target() : Input vector had "
			<< target.size() << " elements, but the robot has " << this->n << " joints."
			<< " Joint target has not been set." << std::endl;
			
		return false;
	}
	else
	{
		float dt;								// Change in time
		float dq;								// Change in position
		float startTime = 0.0;							// Have to declare this here or it won't compile???
		float endTime = 1.0;							// Default trajectory time
		
		for(int i = 0; i < n; i++)
		{
			// Check that the target is within joint limits
			if(target[i] <= this->pLim[i][0]) target[i] = this->pLim[i][0] + 0.01; // Just a bit above the lower limit
			if(target[i] >= this->pLim[i][1]) target[i] = this->pLim[i][1] - 0.01; // Just a bit below the upper limit
	
			// Compute the optimal time scaling for quintic polynomial. See:
			// Angeles, J. (2002). Fundamentals of robotic mechanical systems (Vol. 2).
			// New York: Springer-Verlag.
			dq = abs(target[i] - this->robot.get_joint_position(i));	// Distance travelled
			dt = (15*dq)/(8*this->vLim[i]);				// Time to travel distance at peak velocity
			if(dq != 0 && dt > endTime) endTime = dt;			// Set new end time for trajectory
		}
		
		// Set new trajectory object
		this->jointTrajectory = MultiPointTrajectory(this->robot.get_joint_positions(), target, startTime, endTime);
		
		return true;
	}
}

/******************** Set several waypoints for the joints to move through ********************/
bool SerialKinCtrl::set_joint_targets(const std::vector<Eigen::VectorXf> &targets, const std::vector<float> &times)
{
	std::cout << "\nWorker bees can leave." << std::endl;
	std::cout << "Even drones can fly away." << std::endl;
	std::cout << "The Queen is their slave.\n" << std::endl;
	std::cout << "\n (I need to program in optimal time scaling for this still) \n" << std::endl;
	return false;
}

/******************** Set proportional gains for the feedback control ********************/
bool SerialKinCtrl::set_feedback_gain(const float &gain)
{
	if(gain < 0)
	{
		std::cerr << "[ERROR] [SERIALKINCTRL] set_feedback_gain() : Value cannot be negative." << std::endl;
		std::cerr << " Input value: " << gain << std::endl;
		return false;
	}
	else
	{
		this->k = gain;
		return true;
	}
}

/******************** Set a pose for the end-effector to move to ********************/
bool SerialKinCtrl::set_target_pose(Eigen::Isometry3f &target, float &time)
{
	if(time <= 0)
	{
		std::cerr << "[ERROR] [SERIALKINCTRL] set_target_pose() : Time must be greater than zero." << std::endl;
		std::cerr << "Input time: " << time << std::endl;
		return false;
	}
	else
	{
		// Variables used in this scope
		Eigen::Isometry3f current = this->robot.get_endpoint_pose();			// Current end-effector pose
		Eigen::AngleAxisf dR(current.rotation().inverse()*target.rotation());	// Difference in orientation
		float distance = (target.translation() - current.translation()).norm();	// Difference in position
		float maxSpeed;
		
		// Ensure linear velocity isn't too high
		maxSpeed = 1.0;							// m/s
		if(distance / time > maxSpeed)
		{
			std::cerr << "[WARNING] [SERIALKINCTRL] set_target_pose() : Linear velocity exceeds "
				<< maxSpeed << " m/s! Increasing the trajectory time..." << std::endl;
			time = distance/maxSpeed;
		}
		
		// Ensure angular velocity isn't too high
		maxSpeed = 10.5;							// rad/s (100 RPM)
		distance = dR.angle();
		if(distance > M_PI) distance = 2*M_PI - distance;			// Shortest path (should be corrected in trajectory object)
		if(distance / time > maxSpeed)
		{
			std::cerr << "[WARNING] [SERIALKINCTRL] set_target_pose() : Angular velocity exceeds "
				<< maxSpeed*30/M_PI << " RPM! Increasing the trajectory time..." << std::endl;
			time = distance/maxSpeed;
		}
		
		this->cartesianTrajectory = CartesianTrajectory(current, target, 0.0, time); // Set new trajectory

		return true;
	}
}

/******************** Set multiple waypoints for the end-effector to move to ********************/
bool SerialKinCtrl::set_target_poses(const std::vector<Eigen::Isometry3f> &targets, const std::vector<float> &times)
{
	std::cout << "\nWorker bees can leave." << std::endl;
	std::cout << "Even drones can fly away." << std::endl;
	std::cout << "The Queen is their slave.\n" << std::endl;
	std::cout << "\n (I need to program in optimal time scaling for this still) \n" << std::endl;
	return false;
}

/******************** Get the joint velocity to track the internal trajectory ********************/
Eigen::VectorXf SerialKinCtrl::get_cartesian_control(const float &time)
{
	// Get the desired state for the endpoint
	Eigen::Isometry3f x_d; Eigen::VectorXf xdot_d(6), xddot_d(6);
	this->cartesianTrajectory.get_state(x_d, xdot_d, xddot_d, time);		// Desired state for give time
	
	// Compute the mapping from Cartesian to joint space
	Eigen::MatrixXf J = this->robot.get_jacobian();				// Get the Jacobian
	Eigen::MatrixXf W = get_joint_limit_weighting(); 				// NOTE: THIS IS CURRENTLY THE INVERSE!
	Eigen::MatrixXf invWJt = W*J.transpose();					// Makes things a little faster
	Eigen::MatrixXf invJ = invWJt*get_inverse(J*invWJt);				// Weighted pseudoinverse

	// Compute the joint velocities to achieve the endpoint motion
	Eigen::VectorXf qdot_R = invJ*(xdot_d + this->k*get_pose_error(x_d, this->robot.get_endpoint_pose())); // Range space vector
	scale_velocity_vector(qdot_R, qdot_R);					// Ensure feasiblity of the range space vector
	
	if(this->n <= 6) return qdot_R;						// No redundancy available
	else										// Add a redundant task
	{
		Eigen::MatrixXf N = Eigen::MatrixXf::Identity(this->n, this->n) - invJ*J; // Null space projection matrix
		Eigen::VectorXf qdot_N(this->n);					// Desired joint velocity

//		NOTE TO SELF: Need to expand this later.
//		switch(redunantTask)
//		{
//			case: manipulability
//			{
				qdot_N = N*W*optimise_manipulability(0.5, J);		// This is really N*inv(W)*qdot_d;
//				break;
//			}
//			default:
//			{
//				¯\_(ツ)_/¯
//				break;
//			}
//		}
		
		Eigen::VectorXf qdot = qdot_R + qdot_N;				// Add the range space and null space vectors together
		scale_velocity_vector(qdot_N, qdot);					// Scale null space vector to ensure feasibility
		return qdot_R + qdot_N;						// Recombine after scaling
	}
}

/******************** Get the inverse of a matrix, add damping as necessary ********************/
Eigen::MatrixXf SerialKinCtrl::get_inverse(const Eigen::MatrixXf &A)
{
	Eigen::JacobiSVD<Eigen::MatrixXf> SVD(A, Eigen::ComputeFullU | Eigen::ComputeFullV); // Get the SVD decomposition
	Eigen::MatrixXf V = SVD.matrixV();						// V-matrix
	Eigen::VectorXf s = SVD.singularValues();					// Get the singular values
	Eigen::MatrixXf invA(A.cols(), A.rows()); invA.setZero();			// Value we want to return

	for(int i = 0; i < A.cols(); i++)
	{
		for(int j = 0; j < A.rows(); j++)
		{
			if(s(j) > 1e-06)	invA(i,j) += V(i,j)/s(j);		// Left half of the inverse
//			else			invA(i,j) += V(i,j)*0			// Ignore singular directions
		}
	}
	
	return invA*SVD.matrixU().transpose();					// Right half of the inverse and return
}

/******************** Get the weighted inverse of a matrix ********************/
Eigen::MatrixXf SerialKinCtrl::get_inverse(const Eigen::MatrixXf &A, const Eigen::MatrixXf &W)
{
	// Matrix isn't square, return zero
	if(W.cols() != W.rows())
	{
		std::cout << "ERROR: SerialKinCtrl::get_inverse() : Cannot compute the weighted inverse!"
			<< " Weighting matrix is " << W.rows() << "x" << W.cols() << ", but it must be square." << std::endl;
			
		return Eigen::MatrixXf::Zero(A.cols(), A.rows());
	}
	// Columns don't match rows, return zero
	else if(A.cols() != W.rows())
	{
		std::cout << "ERROR: SerialKinCtrl::get_inverse() :: Cannot compute the weighted inverse!"
			<< " Input matrix has " << A.cols() << " columns and weighting matrix has " << W.rows()
			<< " rows, but they must be the same." << std::endl;
		return Eigen::MatrixXf::Zero(A.cols(), A.rows());
	}
	else
	{	Eigen::MatrixXf invW = get_inverse(W);				// Get the inverse of the weighting matrix
		Eigen::MatrixXf invWAt = invW*A.transpose();				// This makes calcs a little faster
		return invWAt*get_inverse(A*invWAt);					// Return the inverse
	}
}

/******************** Compute the velocity needed to track the internal trajectory ********************/
Eigen::VectorXf SerialKinCtrl::get_joint_control(const float &time)
{
	Eigen::VectorXf q_d(this->n), qdot_d(this->n), qddot_d(this->n);		// Desired joint position, velocity, acceleration
	
	this->jointTrajectory.get_state(q_d, qdot_d, qddot_d, time); 		// Get the desired state from the trajectory
	
	return qdot_d	+ this->k*(q_d - this->robot.get_joint_positions());		// Feedforward + feedback
}

/******************** Get the error between two poses for feedback control purposes ********************/
Eigen::VectorXf SerialKinCtrl::get_pose_error(const Eigen::Isometry3f &desired,
						const Eigen::Isometry3f &actual)
{
	Eigen::VectorXf error(6);							// Value to be returned
	error.head(3) = desired.translation() - actual.translation();		// Get the translation error
	Eigen::Isometry3f Re = desired*actual.inverse();				// Get the rotation error (is there a better way?)
	error.tail(3) = Eigen::Quaternionf(Re.rotation()).vec();			// Quaternion feedback
	return error;
}

/******************** Weighting matrix for joint limit avoidance ********************/
Eigen::MatrixXf SerialKinCtrl::get_joint_limit_weighting()
{
	// Chan, T. F., & Dubey, R. V. (1995). A weighted least-norm solution based scheme
	// for avoiding joint limits for redundant joint manipulators.
	// IEEE Transactions on Robotics and Automation, 11(2), 286-292.
	
	Eigen::MatrixXf W(this->n, this->n); W.setIdentity();			// Value to be returned
	float q, dpdq, range, upper, lower;
	for(int i = 0; i < this->n; i++)
	{
		q = this->robot.get_joint_position(i);				// Joint position
		upper = this->pLim[i][1] - q;						// Distance to upper limit
		lower = q - this->pLim[i][0];						// Distance to lower limit
		range = this->pLim[i][1] - this->pLim[i][0];				// Difference between upper and lower
		dpdq = (range*range*(2*q - this->pLim[i][1] - this->pLim[i][0]))	// Partial derivative of penalty function
		      /(4*upper*upper*lower*lower);
			
		if(dpdq*this->robot.get_joint_velocity(i) > 0)			// Moving toward a limit
		{
			float penalty = range*range/(4*upper*lower);			// Actual penalty function
			
			if(penalty < 1)
			{
				std::cout << "ERROR: SerialKinCtrl::get_joint_limit_weighting() : "
				<< "Penalty function is less than 1! How did that happen???" << std::endl;
				std::cout << "qMin: " << this->pLim[i][0] << " q: " << q << " qMax: " << this->pLim[i][1] << std::endl;
				penalty = 1.0;
			}
			W(i,i) = 1/penalty; // NOTE: IN FUTURE, DON'T RETURN THE INVERSE!
		}
	}
	return W;
}

/******************** Scale a velocity vector to avoid joint limits ********************/
bool SerialKinCtrl::scale_velocity_vector(Eigen::VectorXf &vec, const Eigen::VectorXf ref)
{
	if(vec.size() != ref.size())
	{
		std::cerr << "[ERROR][SERIALKINCTRL] scale_velocity_vector() : Input vectors are not the same length." << std::endl;
		std::cerr << " vec: " << vec.size() << " ref: " << ref.size() << std::endl;
		return false;
	}
	else
	{
		float s = 1.0;								// Scalar for the velocity vector
		float vMin, vMax;
		for(int i = 0; i < ref.size(); i++)
		{
			vMin = this->vLim[i] *= -1;
			vMax = this->vLim[i];
			
			if(ref[i] < vMin && vMin/ref[i] < s)		s = 0.99*vMin/ref[i]; // Lower than min and largest observed so far...
			else if(ref[i] > vMax && vMax/ref[i] < s)	s = 0.99*vMax/ref[i]; // Higher than max and larget observed so far...
		}
		vec *= s;								// Scale to ensure feasibility
		return true;
	}
}

/******************** Get the gradient of manipulability to avoid singularities ********************/
Eigen::VectorXf SerialKinCtrl::optimise_manipulability(const float &scalar, const Eigen::MatrixXf &J)
{
	// Variables used in this scope
	Eigen::MatrixXf JJt = J*J.transpose();					// This makes things a little easier
	Eigen::MatrixXf invJ = J.transpose()*get_inverse(JJt);			// Pseudoinverse of Jacobian
	Eigen::MatrixXf dJ;								// Partial derivative of the Jacobian
	Eigen::VectorXf grad(this->n);						// Value to be returned
	float mu = sqrt(JJt.determinant());						// Actual measure of manipulability

	if(scalar <= 0)
	{
		std::cerr << "[ERROR] [SERIALKINCTRL] optimise_manipulability () : Scalar " << scalar << " must be positive." << std::endl;
		grad.setZero();							// Don't do anything!
	}
	else
	{
		grad[0] = 0.0;								// First joint doesn't affect manipulability
		
		for(int i = 1; i < this->n; i++)
		{
			dJ = this->robot.get_partial_derivative(J, i);		// Get partial derivative w.r.t. ith joint
			grad[i] = scalar*mu*(dJ*invJ).trace();			// Gradient of manipulability
		}
	}
	
	return grad;
}
