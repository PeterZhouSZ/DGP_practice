#pragma once

#include <vector>

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <Eigen/Geometry>
#include <Eigen/SparseCholesky>

#include <glm/glm.hpp>

namespace DGP{

typedef Eigen::MatrixXd VMat;
typedef Eigen::MatrixXi FMat;
typedef Eigen::VectorXd Vec;
typedef Eigen::SparseMatrix<double> SpMat;
typedef Eigen::Triplet<double> T;

#define VPos(x) glm::dvec3(V(x, 0), V(x, 1), V(x, 2))

	/* compute cot angle between vector a, b,  cot = <a, b> / |a x b| */
	double cot(glm::dvec3 a, glm::dvec3 b)
	{
		double cos = glm::dot(a, b);
		double sin = glm::length(glm::cross(a, b));
		return cos / sin;
	}

	SpMat reverseDiag(Vec& A)
	{
		int size = A.size();

		std::vector<T> coeff;
		coeff.clear();
		coeff.reserve(size);

		for(int i = 0; i < size; i++)
			coeff.push_back(T(i, i, 1.0 / A(i)));

		SpMat I(size, size);
		I.setFromTriplets(coeff.begin(), coeff.end());

		return I;
	}

	/*
	* return Laplacian-Beltra Operator
	* 
	* Args:
	*	V: matrix of vertices positions, V * 3 double
	*   F: matrix of triangle faces,	 F * 3 int
	*
	* Return:
	*	L: sparse matrix of Laplacian-Beltra, V * V sparse double
	*/
	SpMat Laplacian(VMat &V, FMat &F)
	{
		/* size of the mesh */
		int f_num = F.innerSize();
		int v_num = V.innerSize();

		/* initialize triplets */
		std::vector<T> lap_coeff;
		lap_coeff.clear();
		lap_coeff.reserve(f_num * 12);

		for (int i = 0; i < f_num; i++)
		{
			glm::ivec3 v_id = glm::ivec3(F(i, 0), F(i, 1), F(i, 2));
			glm::dmat3 v_pos = glm::dmat3(VPos(v_id[0]), VPos(v_id[1]), VPos(v_id[2]));

			for (int j = 0; j < 3; j++)
			{
				double cot_ij;
				/* compute cot of ij */
				cot_ij = cot(v_pos[j] - v_pos[(j + 2) % 3], v_pos[(j + 1) % 3] - v_pos[(j + 2) % 3]);
				lap_coeff.push_back(T(v_id[j],			 v_id[(j + 1) % 3],  cot_ij));
				lap_coeff.push_back(T(v_id[(j + 1) % 3], v_id[j],			 cot_ij));
				lap_coeff.push_back(T(v_id[j],			 v_id[j],		    -cot_ij));
				lap_coeff.push_back(T(v_id[(j + 1) % 3], v_id[(j + 1) % 3], -cot_ij));
			}
		}

		SpMat Lap(v_num, v_num);
		Lap.setFromTriplets(lap_coeff.begin(), lap_coeff.end());

		return Lap;
	}


	/* reverse Area of each vertex */
	Vec Area(VMat &V, FMat &F)
	{
		/* size of the mesh */
		int f_num = F.innerSize();
		int v_num = V.innerSize();

		Vec A = Vec::Zero(v_num);

		for(int i = 0; i < f_num; i++)
		{
			glm::ivec3 v_id = glm::ivec3(F(i, 0), F(i, 1), F(i, 2));
			glm::dmat3 v_pos = glm::dmat3(VPos(v_id[0]), VPos(v_id[1]), VPos(v_id[2]));

			double s = glm::length(glm::cross(v_pos[0] - v_pos[2], v_pos[1] - v_pos[2]));
			s /= 6;

			A(v_id[0]) += s;
			A(v_id[1]) += s;
			A(v_id[2]) += s;
		}

	
		return A;
	}

	VMat smoothMesh(VMat& V, FMat& F, double h)
	{
		SpMat Lap = Laplacian(V, F);
		Vec A = Area(V, F);
		/* compute  H = I - h * L */
		SpMat H = SpMat(Lap.innerSize(), Lap.outerSize());
		H.setIdentity();
		//H = H - h * reverseDiag(A) * Lap;
		H = H - h * Lap;
		
		/* solve H V_t = V */
		Eigen::SimplicialLDLT<SpMat> solver;
		solver.compute(H);

		VMat new_V = solver.solve(V);

		return new_V;
	}
}
