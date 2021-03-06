/////////////////////////////////////////////////////////////////
// (c) Copyright 2007-  Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   Modified by Jeongnim Kim for qmcpack
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
/** @file Bspline3DSet.h
 * @brief Define Bspline3DSetBase and its derived classes
 *
 * - Bspline3DSet_Ortho : orthorhombic unit cell
 * - Bspline3DSet_Gen : non-orthorhombic unit cell
 * - Bspline3DSet_Ortho_Trunc: orthorhombic unit cell with localized orbitals
 * - Bspline3DSet_Gen_Trunc : non-orthorhombic unit cell with localized orbitals
 */
#ifndef TRICUBIC_BSPLINE3D_SINGLEORBITALSET_BASE_H
#define TRICUBIC_BSPLINE3D_SINGLEORBITALSET_BASE_H

#include "Configuration.h"
#include "QMCWaveFunctions/SPOSetBase.h"
#include "Numerics/TricubicBsplineGrid.h"
#include "Optimize/VarList.h"

namespace qmcplusplus {

  /** base class for Bspline3DSet<bool ORTHO, bool TRUNC> and Bspline3DSet_XYZ
  */
  struct Bspline3DSetBase: public SPOSetBase
  {

    typedef TricubicBsplineGrid<ValueType>  GridType;
    typedef Array<ValueType,DIM>      StorageType;
    typedef CrystalLattice<RealType,DIM> UnitCellType;
    ///boolean 
    bool Orthorhombic;
    ///number of orbitals
    int NumOrbitals;
    ///index to keep track this object
    int ObjectID;
    ///cutoff radius
    RealType Rcut2;
    ///-|K|^2
    RealType mK2;
    ///TwistAngle in Cartesian Coordinate
    PosType TwistAngle;
    ///number of copies for each direction: only valid with localized orbitals
    TinyVector<int,DIM> Ncopy;
    ///metric tensor to handle generic unitcell
    Tensor<RealType,DIM> GGt;
    ///Lattice
    UnitCellType Lattice;
    ///unitcell for tiling
    UnitCellType UnitLattice;
    ///grid for orbitals
    GridType bKnots;
    ///centers
    vector<PosType> Centers;
    ///displacement vectors for the grid of localized orbitals
    vector<PosType> Origins;
    ///bspline data
    std::vector<const StorageType*> P;

    ///default constructor
    Bspline3DSetBase();

    ///virtual destructor
    virtual ~Bspline3DSetBase();

    /** reset optimizable variables. 
     *
     * Currently nothing is optimized.
     */
    void checkInVariables(opt_variables_type& active);
    void checkOutVariables(const opt_variables_type& active);
    void resetParameters(const opt_variables_type& active);
    void reportStatus(ostream& os);
    void resetTargetParticleSet(ParticleSet& e);

    inline void setRcut(RealType rc)
    {
      Rcut2=rc*rc;
    }

    /** set the lattice of the spline sets */
    void setLattice(const CrystalLattice<RealType,DIM>& lat);
    /** resize the internal storage P and Centers
     * @param norbs number of orbitals of this set
     */
    void resize(int norbs);
    /** set the twist angle 
     * @param tangle twist angle in Cartesian
     */
    void setTwistAngle(const PosType& tangle);
    /** set the grid
     * @param knots copy the grid
     */
    void setGrid(const GridType& knots);
    /** set the grid
     */
    void setGrid(RealType xi, RealType xf, 
        RealType yi, RealType yf, RealType zi, RealType zf, 
        int nx, int ny, int nz, 
        bool pbcx, bool pbcy, bool pbcz, bool openend);

    /** add a bspline orbital
     * @param i index of the orbital
     * @param data input data
     * @param curP interpolated data
     */
    void add(int i, const StorageType& data, StorageType* curP);

    /** add a bspline orbital
     * @param i index of the orbital
     * @param curP interpolated data
     */
    void add(int i, StorageType* curP);

    void setOrbitalSetSize(int norbs);

    /** tile localized orbitals
     * @param boxdup duplication of the unitcell for the localized orbitals
     *
     * Added to build a super set of localized orbitals. Centers are displaced
     * by the UnitLattice times integer multiples.
     */
    void tileOrbitals(const TinyVector<int,3>& boxdup);
  };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 2013 $   $Date: 2007-05-22 16:47:09 -0500 (Tue, 22 May 2007) $
 * $Id: TricubicBsplineSet.h 2013 2007-05-22 21:47:09Z jnkim $
 ***************************************************************************/
