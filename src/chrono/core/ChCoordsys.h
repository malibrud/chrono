//
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================

#ifndef CHCOORDSYS_H
#define CHCOORDSYS_H

#include "chrono/core/ChQuaternion.h"
#include "chrono/core/ChVector.h"

namespace chrono {

///
/// COORDSYS:
///
///  This class contains both translational variable
/// (the origin of the axis) and the rotational variable
/// (that is the unitary quaternion which represent the
/// special-orthogonal transformation matrix).
///   Basic features for point-coordinate transformations
/// are provided. However, for more advanced features, the
/// heavier classes ChFrame() or ChFrameMoving() may suit better.
///  The coordsys object comes either with the template "ChCoordsys<type>" mode,
/// either in the 'shortcut' flavor, that is "Coordsys", which assumes
/// the type of the four scalars is double precision, so it is faster to type.
///
/// Further info at the @ref coordinate_transformations manual page.

template <class Real = double>
class ChCoordsys {
  public:
    ChVector<Real> pos;
    ChQuaternion<Real> rot;

  public:
    // Default constructor (identity frame)
    ChCoordsys() : pos(ChVector<Real>(0, 0, 0)), rot(ChQuaternion<Real>(1, 0, 0, 0)){};

    // Construct from position and rotation (as quaternion)
    explicit ChCoordsys(const ChVector<Real>& mv, const ChQuaternion<Real>& mq = ChQuaternion<Real>(1, 0, 0, 0))
        : pos(mv), rot(mq){};

    // Construct from position mv and rotation of angle alpha around unit vector mu
    explicit ChCoordsys(const ChVector<Real>& mv, const Real alpha, const ChVector<Real>& mu) : pos(mv) {
        rot.Q_from_AngAxis(alpha, mu);
    };

	// Construct from triad.  Origin, xdir-origin defines x direction, ydir-origin lies in x-y plane (not co-linear with x-origin).
	explicit ChCoordsys(const ChVector<Real>& origin, const ChVector<Real>& xdir, const ChVector<Real>& ydir) {
		ChVector<Real> ux, uy, uz;

		// Get orthonormal vectors from points provided
		ux = xdir - origin;
		ux.Normalize();
		uz = ux % (ydir - origin);
		uz.Normalize();
		uy = uz % ux;
		uy.Normalize();

		Real R[3][3];
		R[0][0] = ux.x(); R[0][1] = uy.x(); R[0][2] = uz.x();
		R[1][0] = ux.y(); R[1][1] = uy.y(); R[1][2] = uz.y();
		R[2][0] = ux.z(); R[2][1] = uy.z(); R[2][2] = uz.z();

		Real e[4];

		Real trace = R[0][0] + R[1][1] + R[2][2];

		if (trace >= 0) {
			// Rotation angle is not close to pi
			Real s = (Real)sqrt(1 + trace) * 2;
			e[0] = s / 4;
			e[1] = (R[1][2] - R[2][1]) / s;
			e[2] = (R[2][0] - R[0][2]) / s;
			e[3] = (R[0][1] - R[1][0]) / s;
		}
		else
		{
			// Rotation angle is close to pi
			// See: http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
			int i0 = getMaxIdx(R[0][0], R[1][1], R[2][2]);
			int i1 = (i0 + 1) % 3;
			int i2 = (i1 + 1) % 3;
			Real s = (Real)sqrt(1 + R[i0][i0] - R[i1][i1] - R[i2][i2]) * 2;
			e[0] = (R[i2][i1] - R[i1][i2]) / s;
			e[i0 + 1] = s / 4;
			e[i1 + 1] = (R[i0][i1] + R[i1][i0]) / s;
			e[i2 + 1] = (R[i0][i2] + R[i2][i0]) / s;
		}
		pos.Set(origin);
		rot.Set(e[0], e[1], e[2], e[3]);
	}
	static int getMaxIdx(Real x0, Real x1, Real x2) {
		if (x0 >= x1 && x0 >= x2) return 0;
		else if (x1 >= x2) return 1;
		else return 2;
	}

    /// Copy constructor
    ChCoordsys(const ChCoordsys<Real>& other) : pos(other.pos), rot(other.rot){};

    //
    // OPERATORS OVERLOADING
    //

    /// Assignment operator: copy from another coordsys
    ChCoordsys<Real>& operator=(const ChCoordsys<Real>& other) {
        if (&other == this)
            return *this;
        pos = other.pos;
        rot = other.rot;
        return *this;
    }

    bool operator<=(const ChCoordsys<Real>& other) const { return rot <= other.rot && pos <= other.pos; };
    bool operator>=(const ChCoordsys<Real>& other) const { return rot >= other.rot && pos >= other.pos; };

    bool operator==(const ChCoordsys<Real>& other) const { return rot == other.rot && pos == other.pos; }
    bool operator!=(const ChCoordsys<Real>& other) const { return rot != other.rot || pos != other.pos; }

    /// The '>>' operator transforms a coordinate system, so
    /// transformations can be represented with this syntax:
    ///  new_frame = old_frame >> tr_frame;
    /// For a sequence of transformations, i.e. a chain of coordinate
    /// systems, you can also write this (like you would do with
    /// a sequence of Denavitt-Hartemberg matrix multiplications,
    /// but in the _opposite_ order...)
    ///  new_frame = old_frame >> frame3to2 >> frame2to1 >> frame1to0;
    /// This operation is not commutative.
    ChCoordsys<Real> operator>>(const ChCoordsys<Real>& Fb) const { return Fb.TransformLocalToParent(*this); }

    /// The '>>' operator transforms a vector, so transformations
    /// can be represented with this syntax:
    ///  new_v = old_v >> tr_frame;
    /// For a sequence of transformations, i.e. a chain of coordinate
    /// systems, you can also write this (like you would do with
    /// a sequence of Denavitt-Hartemberg matrix multiplications,
    /// but in the opposite order...)
    ///  new_v = old_v >> frame3to2 >> frame2to1 >> frame1to0;
    /// This operation is not commutative.
    // friend ChVector<Real> operator >> (const ChVector<Real>& V, const ChCoordsys<Real>& mcsys)
    //	{
    //		return mcsys.TransformLocalToParent(V);
    //	}

    /// The '*' operator transforms a coordinate system, so
    /// transformations can be represented with this syntax:
    ///  new_frame = tr_frame * old_frame;
    /// For a sequence of transformations, i.e. a chain of coordinate
    /// systems, you can also write this (just like you would do with
    /// a sequence of Denavitt-Hartemberg matrix multiplications!)
    ///  new_frame = frame1to0 * frame2to1 * frame3to2 * old_frame;
    /// This operation is not commutative.
    ///  NOTE: since c++ operator execution is from left to right, in
    /// case of multiple transformations like w=A*B*C*v, the >> operator
    /// performs faster, like  w=v>>C>>B>>A;
    ChCoordsys<Real> operator*(const ChCoordsys<Real>& Fb) const { return this->TransformLocalToParent(Fb); }

    /// The '*' operator transforms a vector, so
    /// transformations can be represented with this syntax:
    ///  new_v = tr_frame * old_v;
    /// For a sequence of transformations, i.e. a chain of coordinate
    /// systems, you can also write this (just like you would do with
    /// a sequence of Denavitt-Hartemberg matrix multiplications!)
    ///  new_v = frame1to0 * frame2to1 * frame3to2 * old_v;
    /// This operation is not commutative.
    ///  NOTE: since c++ operator execution is from left to right, in
    /// case of multiple transformations like w=A*B*C*v, the >> operator
    /// performs faster, like  w=v>>C>>B>>A;
    // ChVector<Real> operator * (const ChVector<Real>& V) const
    //	{
    //			return this->TransformLocalToParent(V);
    //	}

    /// The '/' is like the '*' operator (see), but uses the inverse
    /// transformation for A, in A/b. (with A ChFrame, b ChVector)
    /// That is: c=A*b ; b=A/c;
    ChVector<Real> operator/(const ChVector<Real>& V) const { return TransformParentToLocal(V); }

    /// Performs pre-multiplication of this frame by another
    /// frame, for example: A>>=T means  A'=T*A ; or A'=A >> T
    ChCoordsys<Real>& operator>>=(const ChCoordsys<Real>& T) {
        ConcatenatePreTransformation(T);
        return *this;
    }

    /// Performs post-multiplication of this frame by another
    /// frame, for example: A*=T means  A'=A*T ; or A'=T >> A
    ChCoordsys<Real>& operator*=(const ChCoordsys<Real>& T) {
        ConcatenatePostTransformation(T);
        return *this;
    }

    // Mixed type operators:

    /// Performs pre-multiplication of this frame by a vector D, to 'move' by a displacement D:
    ChCoordsys<Real>& operator>>=(const ChVector<Real>& D) {
        this->pos += D;
        return *this;
    }
    /// Performs pre-multiplication of this frame by a quaternion R, to 'rotate' it by R:
    ChCoordsys<Real>& operator>>=(const ChQuaternion<Real>& R) {
        this->pos = R.Rotate(this->pos);
        this->rot = R * this->rot;
        return *this;
    }

    //
    // FUNCTIONS
    //

    /// Force to z=0, and z rotation only. No normalization to quaternion, however.
    void Force2D() {
        pos.z() = 0;
        rot.e1() = 0;
        rot.e2() = 0;
    }

    /// Returns true if coordsys is identical to other coordsys
    bool Equals(const ChCoordsys<Real>& other) const { return rot.Equals(other.rot) && pos.Equals(other.pos); }

    /// Returns true if coordsys is equal to other coordsys, within a tolerance 'tol'
    bool Equals(const ChCoordsys<Real>& other, Real tol) const {
        return rot.Equals(other.rot, tol) && pos.Equals(other.pos, tol);
    }

    /// Sets to no translation and no rotation
    void SetIdentity() {
        pos = ChVector<Real>(0, 0, 0);
        rot = ChQuaternion<Real>(1, 0, 0, 0);
    }

	/// Cycles the rotation axes by the given number.  
	ChCoordsys<Real>& Cycle(int n)
	{
		int steps;  // 0, 1 or 2
		if (n < 0)  steps = (n - 3 * n) % 3;
		else steps = n % 3;
		Real e[3];
		if (steps == 0) /* Do nothing */;
		if (steps == 1) rot.SetVector(ChVector<Real>(rot.e3(), rot.e1(), rot.e2()));
		if (steps == 2) rot.SetVector(ChVector<Real>(rot.e2(), rot.e3(), rot.e1()));
		return *this;
	}

    // FUNCTIONS TO TRANSFORM THE FRAME ITSELF

    /// Apply a transformation (rotation and translation) represented by
    /// another ChCoordsys T. This is equivalent to pre-multiply this csys
    /// by the other csys T:   this'= T * this; or this' = this >> T
    void ConcatenatePreTransformation(const ChCoordsys<Real>& T) {
        this->pos = T.TransformLocalToParent(this->pos);
        this->rot = T.rot * this->rot;
    }

    /// Apply a transformation (rotation and translation) represented by
    /// another ChCoordsys T in local coordinate. This is equivalent to
    /// post-multiply this csys by the other csys T:   this'= this * T; or this'= T >> this
    void ConcatenatePostTransformation(const ChCoordsys<Real>& T) {
        this->pos = this->TransformLocalToParent(T.pos);
        this->rot = this->rot * T.rot;
    }

    // FUNCTIONS FOR COORDINATE TRANSFORMATIONS

    /// This function transforms a point from the local coordinate
    /// system to the parent coordinate system. Relative position of local respect
    /// to parent is given by this coordys, i.e. 'origin' translation and 'alignment' quaternion.
    /// \return The point in parent coordinate, as parent=origin +q*[0,(local)]*q'
    ChVector<Real> TransformLocalToParent(const ChVector<Real>& local) const { return pos + rot.Rotate(local); }

    ChVector<Real> TransformPointLocalToParent(const ChVector<Real>& local) const { return pos + rot.Rotate(local); }

    /// This function transforms a point from the parent coordinate
    /// system to a local coordinate system, whose relative position
    /// is given by this coodsys, i.e. 'origin' translation and 'alignment' quaternion.
    /// \return The point in local coordinate, as local=q'*[0,(parent-origin)]*q
    ChVector<Real> TransformParentToLocal(const ChVector<Real>& parent) const { return rot.RotateBack(parent - pos); }

    ChVector<Real> TransformPointParentToLocal(const ChVector<Real>& parent) const {
        return rot.RotateBack(parent - pos);
    }

    /// This function transforms a direction from 'this' local coordinate system
    /// to the parent coordinate system.
    ChVector<Real> TransformDirectionLocalToParent(const ChVector<Real>& local) const { return rot.Rotate(local); }

    /// This function transforms a direction from the parent coordinate system to
    /// 'this' local coordinate system.
    ChVector<Real> TransformDirectionParentToLocal(const ChVector<Real>& parent) const {
        return rot.RotateBack(parent);
    }

    /// This function transforms a coordsys given in 'this' coordinate system to
    /// the parent coordinate system
    ChCoordsys<Real> TransformLocalToParent(const ChCoordsys<Real>& local) const {
        return ChCoordsys<Real>(TransformLocalToParent(local.pos), rot % local.rot);
    }

    /// This function transforms a coordsys given in the parent coordinate system
    /// to 'this' coordinate system
    ChCoordsys<Real> TransformParentToLocal(const ChCoordsys<Real>& parent) const {
        return ChCoordsys<Real>(TransformParentToLocal(parent.pos), rot.GetConjugate() % parent.rot);
    }

    //
    // STREAMING
    //

    /// Method to allow serialization of transient data in archives.
    void ArchiveOUT(ChArchiveOut& marchive) {
        // suggested: use versioning
        marchive.VersionWrite<ChCoordsys<double>>();
        // stream out all member data
        marchive << CHNVP(pos);
        marchive << CHNVP(rot);
    }

    /// Method to allow de-serialization of transient data from archives.
    void ArchiveIN(ChArchiveIn& marchive) {
        // suggested: use versioning
        int version = marchive.VersionRead<ChCoordsys<double>>();
        // stream in all member data
        marchive >> CHNVP(pos);
        marchive >> CHNVP(rot);
    }
};

CH_CLASS_VERSION(ChCoordsys<double>, 0)

//
// MIXED ARGUMENT OPERATORS
//

// Mixing with ChVector :

/// The '*' operator that transforms 'mixed' types:
///  vector_C = frame_A * vector_B;
/// where frame_A  is  a ChCoordsys
///       vector_B is  a ChVector
/// Returns a ChVector.
/// The effect is like applying the transformation frame_A to vector_B and get vector_C.
template <class Real>
ChVector<Real> operator*(const ChCoordsys<Real>& Fa, const ChVector<Real>& Fb) {
    return Fa.TransformPointLocalToParent(Fb);
}

/// The '*' operator that transforms 'mixed' types:
///  frame_C = vector_A * frame_B;
/// where vector_A is  a ChVector
///       frame_B  is  a ChCoordsys
/// Returns a ChCoordsys.
/// The effect is like applying the translation vector_A to frame_B and get frame_C.
template <class Real>
ChCoordsys<Real> operator*(const ChVector<Real>& Fa, const ChCoordsys<Real>& Fb) {
    ChCoordsys<Real> res(Fb);
    res.pos += Fa;
    return res;
}

/// The '>>' operator that transforms 'mixed' types:
///  vector_C = vector_A >> frame_B;
/// where vector_A is  a ChVector
///       frame_B  is  a ChCoordsys
/// Returns a ChVector.
/// The effect is like applying the transformation frame_B to frame_A and get frame_C.
template <class Real>
ChVector<Real> operator>>(const ChVector<Real>& Fa, const ChCoordsys<Real>& Fb) {
    return Fb.TransformPointLocalToParent(Fa);
}

/// The '>>' operator that transforms 'mixed' types:
///  frame_C = frame_A >> vector_B;
/// where frame_A  is  a ChCoordsys
///       vector_B is  a ChVector
/// Returns a ChCoordsys.
/// The effect is like applying the translation vector_B to frame_A and get frame_C.
template <class Real>
ChCoordsys<Real> operator>>(const ChCoordsys<Real>& Fa, const ChVector<Real>& Fb) {
    ChCoordsys<Real> res(Fa);
    res.pos += Fb;
    return res;
}

// Mixing with ChQuaternion :

/// The '*' operator that transforms 'mixed' types:
///  quat_C = frame_A * quat_B;
/// where frame_A  is  a ChCoordsys
///       quat_B   is  a ChQuaternion
/// Returns a ChQuaternion.
/// The effect is like applying the transformation frame_A to quat_B and get quat_C.
template <class Real>
ChQuaternion<Real> operator*(const ChCoordsys<Real>& Fa, const ChQuaternion<Real>& Fb) {
    return Fa.rot * Fb;
}

/// The '*' operator that transforms 'mixed' types:
///  frame_C = quat_A * frame_B;
/// where quat_A   is  a ChQuaternion
///       frame_B  is  a ChCoordsys
/// Returns a ChCoordsys.
/// The effect is like applying the rotation quat_A to frame_B and get frame_C.
template <class Real>
ChCoordsys<Real> operator*(const ChQuaternion<Real>& Fa, const ChCoordsys<Real>& Fb) {
    ChCoordsys<Real> res(Fa.Rotate(Fb.pos), Fa * Fb.rot);
    return res;
}

/// The '>>' operator that transforms 'mixed' types:
///  quat_C = quat_A >> frame_B;
/// where quat_A   is  a ChQuaternion
///       frame_B  is  a ChCoordsys
/// Returns a ChQuaternion.
/// The effect is like applying the transformation frame_B to quat_A and get quat_C.
template <class Real>
ChQuaternion<Real> operator>>(const ChQuaternion<Real>& Fa, const ChCoordsys<Real>& Fb) {
    return Fa >> Fb.rot;
}

/// The '>>' operator that transforms 'mixed' types:
///  frame_C = frame_A >> quat_B;
/// where frame_A is  a ChCoordsys
///       frame_B is  a ChQuaternion
/// Returns a ChCoordsys.
/// The effect is like applying the rotation quat_B to frame_A and get frame_C.
template <class Real>
ChCoordsys<Real> operator>>(const ChCoordsys<Real>& Fa, const ChQuaternion<Real>& Fb) {
    ChCoordsys<Real> res(Fb.Rotate(Fa.pos), Fa.rot >> Fb);
    return res;
}

/// Shortcut for faster use of typical double-precision coordsys.
///  Instead of writing    "ChCoordsys<double> foo;"   you can write
///  the shorter version   "Coordsys foo;"
///
typedef ChCoordsys<double> Coordsys;

/// Shortcut for faster use of typical single-precision coordsys.
///
typedef ChCoordsys<float> CoordsysF;

//
// STATIC COORDSYS OPERATIONS
//

/// Force 3d coordsys to lie on a XY plane (note: no normaliz. on quat)
ChApi Coordsys Force2Dcsys(const Coordsys& cs);

// CONSTANTS

ChApi extern const ChCoordsys<double> CSYSNULL;
ChApi extern const ChCoordsys<double> CSYSNORM;

}  // end namespace chrono

#endif
