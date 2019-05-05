/*
 * International Chemical Identifier (InChI)
 * Version 1
 * Software version 1.05
 * January 27, 2017
 *
 * The InChI library and programs are free software developed under the
 * auspices of the International Union of Pure and Applied Chemistry (IUPAC).
 * Originally developed at NIST.
 * Modifications and additions by IUPAC and the InChI Trust.
 * Some portions of code were developed/changed by external contributors
 * (either contractor or volunteer) which are listed in the file
 * 'External-contributors' included in this distribution.
 *
 * IUPAC/InChI-Trust Licence No.1.0 for the
 * International Chemical Identifier (InChI)
 * Copyright (C) IUPAC and InChI Trust Limited
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the IUPAC/InChI Trust InChI Licence No.1.0,
 * or any later version.
 *
 * Please note that this library is distributed WITHOUT ANY WARRANTIES
 * whatsoever, whether expressed or implied.
 * See the IUPAC/InChI-Trust InChI Licence No.1.0 for more details.
 *
 * You should have received a copy of the IUPAC/InChI Trust InChI
 * Licence No. 1.0 with this library; if not, please write to:
 *
 * The InChI Trust
 * 8 Cavendish Avenue
 * Cambridge CB1 7US
 * UK
 *
 * or e-mail to alan@inchi-trust.org
 *
 */


#ifndef __IXA_MOL_H__
#define __IXA_MOL_H__

#include "../../../../INCHI_BASE/src/ixa.h"

typedef struct
{
    double           x;
    double           y;
    double           z;
    int              atomic_number;
    int              hydrogens[4];
    int              mass;
    IXA_ATOM_RADICAL radical;
    int              charge;
    int              bond_count;
    IXA_BONDID       bonds[20];
} INCHIMOL_ATOM;


typedef struct
{
    IXA_ATOMID         atom1;
    IXA_ATOMID         atom2;
    IXA_BOND_TYPE      type;
    IXA_DBLBOND_CONFIG config;
    IXA_BOND_WEDGE     wedge_from_atom1;
    IXA_BOND_WEDGE     wedge_from_atom2;
} INCHIMOL_BOND;


typedef struct
{
    IXA_STEREO_TOPOLOGY topology;
    IXA_ATOMID          vertices[4]; /* All currently supported topologies have 4 vertices. */
    void*               central_entity; /* Could be an atom or a bond, depending on topology. */
    IXA_STEREO_PARITY   parity;
} INCHIMOL_STEREO;

/************************************************************************/
/*
    Extended input supporting v. 1.05 extensions: V3000; polymers
    This is a later addition to native IXA intended at this time
    just to be a carrier which transports extension data to GetINCHI()
*/
typedef struct INCHIMOL_SGROUP
{
    int id;                /* it is what is called 'Sgroup number' in CTFILE */
    int type;            /* type as by MDL format (STY) */
    int subtype;        /* subtype as by MDL format (SST) */
    int conn;            /* connection scheme  as by MDL format (SCN) */
    int label;            /* it is what is called 'unique Sgroup identifier' in CTFILE */
    int na;                /* number of atoms in the unit */
    int nb;                /* number of bonds in the unit */
    double xbr1[4];        /* bracket ends coordinates (SDI) */
    double xbr2[4];        /* bracket ends coordinates (SDI) */
    char smt[80];        /* Sgroup Subscript (SMT) */
    int *alist;            /* atoms in the unit (SAL) */
    int *blist;            /* bonds in the unit: as [atom1, atom2, atom1, atom2,.. ] for all bonds (as made from SBL) */
}  INCHIMOL_SGROUP;
typedef struct INCHIMOL_POLYMER
{
    INCHIMOL_SGROUP **units;        /* array of pointers */
    int        n;
} INCHIMOL_POLYMER;
/* Extended input supporting v. 1.05 extensions: V3000; polymers */
typedef struct INCHIMOL_V3000
{
    int n_non_star_atoms;
    int n_star_atoms;
    int *atom_index_orig;    /* index as supplied for atoms */
    int *atom_index_fin;    /* = index or -1 for star atom */
    int n_sgroups;            /* currently, we do not use this. */
    int n_3d_constraints;    /* currently, we do not use this. */
    int n_collections;
    int n_non_haptic_bonds;
    int n_haptic_bonds;
    int **lists_haptic_bonds;/* haptic_bonds[i] is pointer to int* which contains:             */
                            /* bond_type, non-star atom number, nendpts, then endpts themselves */
    /* Enhanced stereo */
    int n_steabs;
    int **lists_steabs;        /* steabs[k][0] - not used                            */
                            /* steabs[k][1] -  number of members in collection    */
                            /* steabs[k][2..] - member atom numbers                */
    int n_sterel;
    int **lists_sterel;        /* sterel[k][0] - n from "STERELn" tag                */
                            /* sterel[k][1] -  number of members in collection    */
                            /* sterel[k][2..] - member atom numbers                */
    int n_sterac;
    int **lists_sterac;        /* sterac[k][0] - n from "STERACn" tag                */
                            /* sterac[k][1] -  number of members in collection    */
                            /* sterac[k][0] - number from "STERACn" tag            */
} INCHIMOL_V3000;
/************************************************************************/

typedef struct
{
    int                    atom_count;
    INCHIMOL_ATOM        *atoms;

    int                    bond_count;
    INCHIMOL_BOND*        bonds;

    int                    stereo_count;
    INCHIMOL_STEREO        *stereos;

    INCHIMOL_POLYMER    *polymer;
    INCHIMOL_V3000        *v3000;

    IXA_BOOL            chiral;
} INCHIMOL;


INCHIMOL* MOL_Unpack(IXA_STATUS_HANDLE hStatus, IXA_MOL_HANDLE    HMol);

IXA_ATOMID MOL_GetBondOtherAtom(IXA_STATUS_HANDLE hStatus,
                                IXA_MOL_HANDLE    hMolecule,
                                IXA_BONDID    vBond,
                                IXA_ATOMID    vAtom);

/****************************************************************************/
/*                    Functions for treating extended input/output            */
int  IXA_MOL_SetExtMolDataByInChIExtInput( IXA_STATUS_HANDLE hStatus,
                                           IXA_MOL_HANDLE    hMolecule,
                                           inchi_Output_Polymer    *polymer,
                                           inchi_Output_V3000        *v3000,
                                           int nat );

void IXA_MOL_ClearExtMolData( INCHIMOL_POLYMER *pd, INCHIMOL_V3000 *v3k );


#endif
