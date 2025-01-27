
#include "Quat.h"
#include "fg_geometry.hxx"
#include "fg_geometry.cxx"
#include <math.h>
#include <limits.h>

Quat::Quat(void)
	{
	}

Quat::~Quat(void)
	{
	}

double Quat::getX()
	{
	return m_x;
	}

double Quat::getY()
	{
	return m_y;
	}
double Quat::getZ()
	{
	return m_z;
	}
double Quat::getW()
	{
	return m_w;
	}

/// Create a quaternion from the angle axis representation where the angle
/// is stored in the axis' length
Quat Quat::fromAngleAxis(const Point3D& axis)
	{
	//create Quaternion
	Quat q;
	Point3D vn(axis);
	if(vn.length() > 0)
	{
		vn.normalize();
	}
	double sinHalfAngle = sin( (0.5 * axis.length() ) );

	q.m_w = cos( (0.5 * axis.length() ) );
	q.m_x = (vn[X] * sinHalfAngle);
	q.m_y = (vn[Y] * sinHalfAngle);
	q.m_z = (vn[Z] * sinHalfAngle);

	q.normalise();

	return q;
	}


/// write the euler angles into the references
void Quat::getEulerRad(double& psi,  double& theta, double& phi) const
	{
		/////////////////////////////////////////////////////
		//  
		//  Algorithm based on the Article "Euler Angle Conversion"
		//  by Ken Shoemake in "Graphics Gems IV", p. 222-229
		//  Academic Press 1994.
		//  
		//  From the 24 possible Conversions that are possible based on
		//  Shoemakes Code the Conversion described in IEEE1278.2 DIS is
		//  EulOrdZYXr with rotations around Z, Y' and X''.
		//	The variables i,j and k are specific to this Conversion
		//  The threshold 1e-5 for cy is based on Edward d'Auvergnes 2014
		//  implementation of Shoemakes code in Python (www.nmr-relax.com).
		//
		/////////////////////////////////////////////////////

		// The following commented Code is from the Article

		//enum QuatPart {X, Y, Z, W};
		//double M[4][4];
		//double Nq = this->m_x * this->m_x + this->m_y * this->m_y + this->m_z * this->m_z + this->m_w * this->m_w;
		//double s = (Nq > 0.0) ? (2.0 / Nq) : 0.0;
		//double xs = this->m_x * s,		ys = this->m_y * s,		zs = this->m_z * s;
		//double wx = this->m_w * xs,		wy = this->m_w * ys,	wz = this->m_w * zs;
		//double xx = this->m_x * xs,		xy = this->m_x * ys,	xz = this->m_x * zs;
		//double yy = this->m_y * ys,		yz = this->m_y * zs,	zz = this->m_z * zs;
		//M[X][X] = 1.0 - (yy + zz); M[X][Y] = xy - wz; M[X][Z] = xz + wy;
		//M[Y][X] = xy + wz; M[Y][Y] = 1.0 - (xx + zz); M[Y][Z] = yz - wy;
		//M[Z][X] = xz - wy; M[Z][Y] = yz + wx; M[Z][Z] = 1.0 - (xx + yy);
		//M[W][X]=M[W][Y]=M[W][Z]=M[X][W]=M[Y][W]=M[Z][W]=0.0; M[W][W]=1.0;
		//int i = 0, j = 1, k = 2;
		//double cy = sqrt(M[i][i]*M[i][i] + M[j][i]*M[j][i]);
		//if (cy > 1e-5)
		//{
		//	psi	  = atan2(M[k][j], M[k][k]);
		//	theta = atan2(-M[k][i], cy);
		//	phi   = atan2(M[j][i], M[i][i]);
		//}
		//else
		//{
		//	psi	  = atan2(-M[j][k], M[j][j]);
		//	theta = atan2(-M[k][i], cy);
		//	phi   = 0;
		//}
		//double temp = psi;
		//psi = phi; 
		//phi = temp;

		//Code below is Shoemakes code reduced to the case specific necessities

		//create some elements Mxx from the rotation Matrix
		double Nq = this->m_x * this->m_x + this->m_y * this->m_y + this->m_z * this->m_z + this->m_w * this->m_w;
		double s = (Nq > 0.0) ? (2.0 / Nq) : 0.0;
		double xs = this->m_x * s,		ys = this->m_y * s,		zs = this->m_z * s;
		double wx = this->m_w * xs,		wy = this->m_w * ys,	wz = this->m_w * zs;
		double xx = this->m_x * xs,		xy = this->m_x * ys,	xz = this->m_x * zs;
		double yy = this->m_y * ys,		yz = this->m_y * zs,	zz = this->m_z * zs;
		double M00 = 1.0 - (yy + zz);
		double M10 = xy + wz, M11 = 1.0 - (xx + zz), M12 = yz - wy;
		double M20 = xz - wy, M21 = yz + wx, M22 = 1.0 - (xx + yy);

		//compute psi, theta, phi from the elements of the rotation Matrix
		double cy = sqrt(M00*M00 + M10*M10);
		if (cy > 1e-5)
		{
			phi	  = atan2(M21, M22);
			theta = atan2(-M20, cy);
			psi   = atan2(M10, M00);
		}
		else
		{
			phi	  = atan2(-M12, M11);
			theta = atan2(-M20, cy);
			psi   = 0;
		}
	}

/// Return a quaternion from euler angles
Quat Quat::fromEulerRad(double psi, double theta, double phi)
  {
	/////////////////////////////////////////////////////
	//  
	//  Algorithm based on the Article "Euler Angle Conversion"
	//  by Ken Shoemake in "Graphics Gems IV", p. 222-229
	//  Academic Press 1994.
	//  
	//  From the 24 possible Conversions that are possible based on
	//  Shoemakes Code the Conversion described in IEEE1278.2 DIS is
	//  EulOrdZYXr with rotations around Z, Y' and X''.
	//	The variables i,j and k are specific to this Conversion.
	//
	/////////////////////////////////////////////////////

	// The following commented Code is from the Article

	//enum QuatPart {X, Y, Z, W};
	//Quat qu;
	//double a[3], ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
	//int i=0, j=1, k=2;
	//double x=psi, y=theta, z=phi;
	//double temp=x; x=z; z=temp;
	//ti = x*0.5;   tj = y*0.5;   th = z*0.5;
	//ci = cos(ti); cj = cos(tj); ch = cos(th);
	//si = sin(ti); sj = sin(tj); sh = sin(th);
	//cc = ci*ch; cs = ci*sh; sc = si*ch; ss=si*sh;
	//a[i] = cj*sc - sj*cs;
	//a[j] = cj*ss + sj*cc;
	//a[k] = cj*cs - sj*sc;
	//qu.m_w = cj*cc + sj*ss;
	//qu.m_x = a[X]; qu.m_y = a[Y]; qu.m_z = a[Z];

	//Code below is Shoemakes code reduced to the case specific necessities

    Quat qu;
	double ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
	ti = phi*0.5;   tj = theta*0.5;   th = psi*0.5;
	ci = cos(ti); cj = cos(tj); ch = cos(th);
	si = sin(ti); sj = sin(tj); sh = sin(th);
	cc = ci*ch; cs = ci*sh; sc = si*ch; ss=si*sh;
	qu.m_x = cj*sc - sj*cs;
	qu.m_y = cj*ss + sj*cc;
	qu.m_z = cj*cs - sj*sc;
	qu.m_w = cj*cc + sj*ss;
	return qu;
  }

Point3D Quat::getAngleAxis(Quat qu){
	//build Point from x,y,z values of quaternion
	// |qp.x|   |qu.m_x|   |e_x|
	// |qp.y| = |qu.m_y| = |e_y| * sin(a/2)
	// |qp.z|   |qu.m_z|   |e_z|
	//with e being the unit vector of the angleaxis
	//and a being the angle of the angleaxis
	Point3D qp = Point3D(qu.m_x, qu.m_y, qu.m_z);
	double a;
	// qu.m_w = cos(a/2) 
	// therefore qu.m_w shouldn't be large then 1
	if (qu.m_w > 1)	// if w>1 acos will produce errors
		qu.normalise();
	// qu.m_w = cos(a/2)   <=>   a = 2 * acos(qu.m_w)
	a = 2 * acos(qu.m_w);
	// normalizing qp reduces it to the unit vector e
	// |e.x|   ||qp.x||   |qp.x|
	// |e.y| = ||qp.y|| = |qp.y| / sqrt( qp.x^2 + qp.y^2 + qp.z^2)
	// |e.z|   ||qp.z||   |qp.z|
	Point3D e = qp; e.normalize();
	// angle is stored in size of angle axis
	// |aa.x|   |e_x|
	// |aa.y| = |e_y| * a
	// |aa.z|   |e_z|
	Point3D aa;
	aa.SetX(e.GetX() * a);
	aa.SetY(e.GetY() * a);
	aa.SetZ(e.GetZ() * a);
	return aa;
	}

void Quat::normalise() {

	double n = sqrt(m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w);

	m_x /= n;
	m_y /= n;
	m_w /= n;
	m_z /= n;

	}

Quat Quat::conjugate(Quat q)
	{
	Quat qConj;
	qConj.m_x = -q.m_x;
	qConj.m_y = -q.m_y;
	qConj.m_z = -q.m_z;
	qConj.m_w = q.m_w;
	qConj.normalise();
	return qConj;
	}

Quat Quat::hamiltonProd(Quat q, Quat e)
	{
	 Quat hamProd;
	 hamProd.m_w = q.m_w*e.m_w - q.m_x*e.m_x - q.m_y*e.m_y - q.m_z*e.m_z;
	 hamProd.m_x = q.m_w*e.m_x + q.m_x*e.m_w + q.m_y*e.m_z - q.m_z*e.m_y;
	 hamProd.m_y = q.m_w*e.m_y - q.m_x*e.m_z + q.m_y*e.m_w + q.m_z*e.m_x;
	 hamProd.m_z = q.m_w*e.m_z + q.m_x*e.m_y - q.m_y*e.m_x + q.m_z*e.m_w;
	 return hamProd;
	}
