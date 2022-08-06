#include <iostream>
#include <QPSolver.h>
#include <time.h>

int main(int argc, char *argv[])
{
	srand((unsigned int) time(0));					                            // Random seed generator
	
	// Variables used in this scope
	int m, n;
	Eigen::MatrixXf A, comparison, W;
	Eigen::VectorXf x, xd, xHat, x0, xMin, xMax, y;
	QPSolver solver;
	clock_t timer;
	float time;
	
	std::cout << "\n************************************************************\n"
	          <<   "*                  UNDERDETERMINED SYSTEMS	           *\n"
	          <<   "************************************************************\n" << std::endl;
	          
	m = 6;
	n = 5;
	A  = Eigen::MatrixXf::Random(m,n);
	x  = Eigen::VectorXf::Random(n);
	y  = A*x;
	W  = Eigen::MatrixXf::Identity(m,m);
	x0 = Eigen::VectorXf::Zero(n);
	
	std::cout << "\nHere is an underdetermined system y = A*x.\n" << std::endl;
	
	std::cout << "\nA:\n" << std::endl;
	std::cout << A << std::endl;
	
	std::cout << "\ny:\n" << std::endl;
	std::cout << y << std::endl;
	
	std::cout << "\nWe can use quadratic programming (QP) to get the best estimate of x.\n" << std::endl;
	
	timer = clock();
	xHat  = QPSolver::least_squares(y,A,W,x0);
	timer = clock() - timer;
	time  = (float)timer/CLOCKS_PER_SEC;
	
	std::cout << "\nHere is the estimate for x:\n" << std::endl;
	std::cout << xHat << std::endl;
	
	std::cout << "\n The error ||y-A*x|| is " << (y-A*xHat).norm() << ". "
	          << "It took " << time*1000 << " ms to solve (" << 1/time << " Hz).\n" << std::endl;
	          
	std::cout << "\n************************************************************\n"
	          <<   "*                    CONSTRAINED SYSTEM                    *\n"
	          <<   "************************************************************\n" << std::endl;
	
	m = 7;
	n = 7;
	A = Eigen::MatrixXf::Random(m,n);
	x = 1.5*Eigen::VectorXf::Random(n);
	y = 1.5*A*x;
	W = Eigen::MatrixXf::Identity(m,m);
	xMin = -5*Eigen::VectorXf::Ones(n);
	xMax =  5*Eigen::VectorXf::Ones(n);
	x0   = 0.5*(xMin + xMax);
	
	std::cout << "\nConsider the problem to minimize ||y-A*x|| for xMin <= x <= xMax.\n" << std::endl;
	
	std::cout << "\nHere is A:\n" << std::endl;
	std::cout << A << std::endl;
	
	std::cout << "\nand y:\n" << std::endl;
	std::cout << y << std::endl;
	
	timer = clock();
	xHat  = solver.least_squares(y,A,W,xMin,xMax,x);
	timer = clock() - timer;
	time  = (float)timer/CLOCKS_PER_SEC;
	
	std::cout << "\nHere is xMin, the estimate for x, and xMax side-by-side:\n" << std::endl;
	comparison.resize(n,4);
	comparison.col(0) = xMin;
	comparison.col(1) = x;
	comparison.col(2) = xHat;
	comparison.col(3) = xMax;
	std::cout << comparison << std::endl;
	
	std::cout << "\nThe error norm ||y-A*x|| is " << (y-A*xHat).norm() << ". "
	          << "It took " << time*1000 << " ms to solve ("
	          << 1/time << " Hz)." << std::endl;
	
	std::cout << "\n************************************************************\n"
	          <<   "*                 OVERDETERMINED SYSTEMS                   *\n"
	          <<   "************************************************************\n" << std::endl;
	
	m = 6;
	n = 7;
	
	A = Eigen::MatrixXf::Random(m,n);
	y = 1.5*A*Eigen::VectorXf::Random(n);
	W = Eigen::MatrixXf::Identity(n,n);
	xMin = -Eigen::VectorXf::Ones(n);
	xMax =  Eigen::VectorXf::Ones(n);
	x0   = 0.5*(xMin + xMax);
	xd   = Eigen::VectorXf::Ones(n);
	
	std::cout << "\nWe can solve over-determined systems. Here is the matrix A, "
	          << "which has more columns than rows:\n" << std::endl;
	std::cout <<  A << std::endl;
	
	std::cout << "\nAnd here is the y vector:\n" << std::endl;
	std::cout << y << std::endl;
	
	timer = clock();
	xHat  = solver.least_squares(xd,W,y,A,xMin,xMax,x0);
	timer = clock() - timer;
	time  = (float)timer/CLOCKS_PER_SEC;
	
	std::cout << "\nHere is xMin, the estimate for x, and xMax side-by-side:\n" << std::endl;
	comparison.resize(n,3);
	comparison.col(0) = xMin;
	comparison.col(1) = xHat;
	comparison.col(2) = xMax;
	std::cout << comparison << std::endl;
	
	std::cout << "\nThe error norm ||y-A*x|| is " << (y-A*xHat).norm() << ". "
	          << "It took " << time*1000 << " ms to solve ("
	          << 1/time << " Hz)." << std::endl;
	
	return 0;
}