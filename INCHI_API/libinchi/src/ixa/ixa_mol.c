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


#include <stdlib.h>
#include <string.h>

#include "../../../../INCHI_BASE/src/mode.h"
#include "../../../../INCHI_BASE/src/ichierr.h"
#include "../../../../INCHI_BASE/src/inchi_api.h"
#include "ixa_mol.h"
#include "ixa_status.h"

static const char* Elements[] =
   {"",
    "H",                                                                                 "He",
    "Li","Be",                                                  "B", "C", "N", "O", "F", "Ne",
    "Na","Mg",                                                  "Al","Si","P", "S", "Cl","Ar",
    "K", "Ca","Sc","Ti","V", "Cr","Mn","Fe","Co","Ni","Cu","Zn","Ga","Ge","As","Se","Br","Kr",
    "Rb","Sr","Y", "Zr","Nb","Mo","Tc","Ru","Rh","Pd","Ag","Cd","In","Sn","Sb","Te","I", "Xe",
    "Cs","Ba","La",
        "Ce","Pr","Nd","Pm","Sm","Eu","Gd","Tb","Dy","Ho","Er","Tm","Yb","Lu",
                   "Hf","Ta","W", "Re","Os","Ir","Pt","Au","Hg","Tl","Pb","Bi","Po","At","Rn",
    "Fr","Ra","Ac",
        "Th","Pa","U", "Np","Pu","Am","Cm","Bk","Cf","Es","Fm","Md","No",
                   "Lr","Rf","Db","Sg","Bh","Hs","Mt","Ds","Rg","Cn","Nh","Fl","Mc","Lv","Ts","Og",
    "Zz"
   };

static int GetStereo(IXA_BOND_TYPE  type,
                     IXA_BOND_WEDGE direction,
                     IXA_BOND_WEDGE reverse_direction)
{
    if (type == IXA_BOND_TYPE_SINGLE /*INCHI_BOND_TYPE_SINGLE*/ )
    {
        switch (direction)
        {
            case IXA_BOND_WEDGE_NONE:
                switch (reverse_direction)
                {
                    case IXA_BOND_WEDGE_NONE:
                        return INCHI_BOND_STEREO_NONE;
                    case IXA_BOND_WEDGE_UP:
                        return INCHI_BOND_STEREO_SINGLE_2UP;
                    case IXA_BOND_WEDGE_DOWN:
                        return INCHI_BOND_STEREO_SINGLE_2DOWN;
                    case IXA_BOND_WEDGE_EITHER:
                        return INCHI_BOND_STEREO_SINGLE_2EITHER;
                }

            case IXA_BOND_WEDGE_UP:
                return INCHI_BOND_STEREO_SINGLE_1UP;
            case IXA_BOND_WEDGE_DOWN:
                return INCHI_BOND_STEREO_SINGLE_1DOWN;
            case IXA_BOND_WEDGE_EITHER:
                return INCHI_BOND_STEREO_SINGLE_1EITHER;
        }
    }

    if (direction == IXA_BOND_WEDGE_NONE)
    {
        return INCHI_BOND_STEREO_NONE;
    }
    else
    {
        return INCHI_BOND_STEREO_DOUBLE_EITHER;
    }
}


static int GetVertexCount(IXA_STATUS_HANDLE   hStatus,
                          IXA_STEREO_TOPOLOGY vTopology)
{
    switch (vTopology)
    {
        case IXA_STEREO_TOPOLOGY_TETRAHEDRON:
        case IXA_STEREO_TOPOLOGY_RECTANGLE:
        case IXA_STEREO_TOPOLOGY_ANTIRECTANGLE:
            return 4;
        default:
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Unexpected stereo topology");
            return 0;
    }
}


static IXA_ATOMID MOL_PackAtom(int vAtomIndex)
{
    return (IXA_ATOMID)((size_t)(vAtomIndex + 1));
}


static IXA_BONDID MOL_PackBond(int vBondIndex)
{
    return (IXA_BONDID)((size_t)(vBondIndex + 1));
}


static IXA_STEREOID MOL_PackStereo(int vStereoIndex)
{
    return (IXA_STEREOID)((size_t)(vStereoIndex + 1));
}


static IXA_BOOL MOL_UnpackAtom(IXA_STATUS_HANDLE hStatus,
                               INCHIMOL*         pMolecule,
                               IXA_ATOMID    vAtom,
                               int*              pAtomIndex)
{
    *pAtomIndex = (size_t)(vAtom) - 1;
    if ((*pAtomIndex < 0) || (*pAtomIndex >= pMolecule->atom_count))
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Atom ID is invalid");
        return IXA_FALSE;
    }

    return IXA_TRUE;
}


static IXA_BOOL MOL_UnpackBond(IXA_STATUS_HANDLE hStatus,
                                 INCHIMOL*          pMolecule,
                                 IXA_BONDID    vBond,
                                 int*               pBondIndex)
{
    *pBondIndex = (size_t)(vBond) - 1;
    if ((*pBondIndex < 0) || (*pBondIndex >= pMolecule->bond_count))
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Bond ID is invalid");
        return IXA_FALSE;
    }

    return IXA_TRUE;
}


static IXA_BOOL MOL_UnpackStereo(IXA_STATUS_HANDLE hStatus,
                                   INCHIMOL*          pMolecule,
                                   IXA_STEREOID  vStereo,
                                   int*               pStereoIndex)
{
    *pStereoIndex = (size_t)(vStereo) - 1;
    if ((*pStereoIndex < 0) || (*pStereoIndex >= pMolecule->stereo_count))
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Stereo ID is invalid");
        return IXA_FALSE;
    }

    return IXA_TRUE;
}


static INCHIMOL_ATOM* MOL_GetAtom(IXA_STATUS_HANDLE hStatus,
                                  INCHIMOL*          pMolecule,
                                  IXA_ATOMID    vAtom)
{
    int atom_index;
    if (!MOL_UnpackAtom(hStatus, pMolecule, vAtom, &atom_index)) return NULL;
    return &pMolecule->atoms[atom_index];
}


static INCHIMOL_BOND* MOL_GetBond(IXA_STATUS_HANDLE hStatus,
                                  INCHIMOL*          pMolecule,
                                  IXA_BONDID    vBond)
{
    int bond_index;
    if (!MOL_UnpackBond(hStatus, pMolecule, vBond, &bond_index)) return NULL;
    return &pMolecule->bonds[bond_index];
}


static INCHIMOL_STEREO* MOL_GetStereo(IXA_STATUS_HANDLE hStatus,
                                      INCHIMOL*          pMolecule,
                                      IXA_STEREOID  vStereo)
{
    int stereo_index;
    if (!MOL_UnpackStereo(hStatus, pMolecule, vStereo, &stereo_index)) return NULL;
    return &pMolecule->stereos[stereo_index];
}


void MOL_Clear(INCHIMOL* pMolecule)
{
    inchi_free(pMolecule->atoms);
    inchi_free(pMolecule->bonds);
    inchi_free(pMolecule->stereos);

    IXA_MOL_ClearExtMolData( pMolecule->polymer, pMolecule->v3000 );

    memset(pMolecule, 0, sizeof(INCHIMOL));
}


static IXA_ATOMID MOL_CreateAtom(IXA_STATUS_HANDLE hStatus,
                                 INCHIMOL* pMolecule)
{
    /*
    Reallocate the atoms array to make space for one more atom. Doing things this way means
    that a reallocation occurs every time MOL_CreateAtom is called, which is inefficient.
    Alternatives include:
    1. Allocating several atoms at a time so that a reallocations occur only when the pool
       of pre-allocated atoms has been exhausted. This is more efficient but also more
       complicated and wastes memory.
    2. Giving INCHIMOL a function equivalent to std::vector::reserve so that the user can
       pre-allocate exactly the number of atoms that they know they will need. This avoids
       wasting memory but is even more complicated.
    */
    INCHIMOL_ATOM* temp = (INCHIMOL_ATOM *) inchi_calloc(pMolecule->atom_count + 1, sizeof(INCHIMOL_ATOM));
    if (!temp)
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
        return IXA_ATOMID_INVALID;
    }

    memcpy(temp, pMolecule->atoms, pMolecule->atom_count * sizeof(INCHIMOL_ATOM));
    inchi_free(pMolecule->atoms);
    pMolecule->atoms = temp;
    pMolecule->atoms[pMolecule->atom_count].x             = 0;
    pMolecule->atoms[pMolecule->atom_count].y             = 0;
    pMolecule->atoms[pMolecule->atom_count].z             = 0;
    pMolecule->atoms[pMolecule->atom_count].atomic_number = 6; /* Carbon */
    pMolecule->atoms[pMolecule->atom_count].hydrogens[0]  = 0;
    pMolecule->atoms[pMolecule->atom_count].hydrogens[1]  = 0;
    pMolecule->atoms[pMolecule->atom_count].hydrogens[2]  = 0;
    pMolecule->atoms[pMolecule->atom_count].hydrogens[3]  = 0;
    pMolecule->atoms[pMolecule->atom_count].mass          = 0;
    pMolecule->atoms[pMolecule->atom_count].radical       = IXA_ATOM_RADICAL_NONE;
    pMolecule->atoms[pMolecule->atom_count].charge        = 0;
    pMolecule->atoms[pMolecule->atom_count].bond_count    = 0;
    return MOL_PackAtom(pMolecule->atom_count++);
}


static int MOL_CreateStereo(IXA_STATUS_HANDLE hStatus,
                            INCHIMOL*          pMolecule)
{
    /*
    Reallocate the atoms array to make space for one more atom. Doing things this way means
    that a reallocation occurs every time MOL_CreateAtom is called, which is inefficient.
    Alternatives include:
    1. Allocating several atoms at a time so that a reallocations occur only when the pool
       of pre-allocated atoms has been exhausted. This is more efficient but also more
       complicated and wastes memory.
    2. Giving INCHIMOL a function equivalent to std::vector::reserve so that the user can
       pre-allocate exactly the number of atoms that they know they will need. This avoids
       wasting memory but is even more complicated.
    */
    INCHIMOL_STEREO* temp = (INCHIMOL_STEREO *) inchi_calloc(pMolecule->stereo_count + 1, sizeof(INCHIMOL_STEREO));
    if (!temp)
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
        return -1;
    }

    memcpy(temp, pMolecule->stereos, pMolecule->stereo_count * sizeof(INCHIMOL_STEREO));
    memset(&temp[pMolecule->stereo_count], 0, sizeof(INCHIMOL_STEREO));
    inchi_free(pMolecule->stereos);
    pMolecule->stereos = temp;
    return pMolecule->stereo_count++;
}


static IXA_MOL_HANDLE MOL_Pack(INCHIMOL* pMolecule)
{
    return (IXA_MOL_HANDLE)pMolecule;
}


INCHIMOL* MOL_Unpack(IXA_STATUS_HANDLE hStatus,
                     IXA_MOL_HANDLE    hMolecule)
{
    if (!hMolecule)
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Illegal molecule handle detected");
        return NULL;
    }

    return (INCHIMOL*)hMolecule;
}


IXA_ATOMID MOL_GetBondOtherAtom(IXA_STATUS_HANDLE hStatus,
                                    IXA_MOL_HANDLE    hMolecule,
                                    IXA_BONDID    vBond,
                                    IXA_ATOMID    vAtom)
{
    IXA_ATOMID atom1;
    IXA_ATOMID atom2;

    atom1 = IXA_MOL_GetBondAtom1(hStatus, hMolecule, vBond);
    if (IXA_STATUS_HasError(hStatus)) return IXA_ATOMID_INVALID;

    atom2 = IXA_MOL_GetBondAtom2(hStatus, hMolecule, vBond);
    if (IXA_STATUS_HasError(hStatus)) return IXA_ATOMID_INVALID;

    if (atom1 == vAtom)
    {
        return atom2;
    }
    else if (atom2 == vAtom)
    {
        return atom1;
    }
    else
    {
        return IXA_ATOMID_INVALID;
    }
}


IXA_MOL_HANDLE INCHI_DECL IXA_MOL_Create(IXA_STATUS_HANDLE hStatus)
{
    INCHIMOL* molecule = (INCHIMOL*)inchi_malloc(sizeof(INCHIMOL));
    if (!molecule)
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
        return NULL;
    }

    memset(molecule, 0, sizeof(INCHIMOL));
    return MOL_Pack(molecule);
}


void INCHI_DECL IXA_MOL_Destroy(IXA_STATUS_HANDLE hStatus,
                                IXA_MOL_HANDLE    hMolecule)
{
    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    MOL_Clear(molecule);
    inchi_free(molecule);
}


void INCHI_DECL IXA_MOL_Clear(IXA_STATUS_HANDLE hStatus,
                              IXA_MOL_HANDLE    hMolecule)
{
    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    MOL_Clear(molecule);
}


void INCHI_DECL IXA_MOL_SetChiral(IXA_STATUS_HANDLE hStatus,
                                  IXA_MOL_HANDLE    hMolecule,
                                  IXA_BOOL          vChiral)
{
    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    molecule->chiral = vChiral;
}


IXA_BOOL INCHI_DECL IXA_MOL_GetChiral(IXA_STATUS_HANDLE hStatus,
                                      IXA_MOL_HANDLE    hMolecule)
{
    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_FALSE;

    return molecule->chiral;
}


IXA_ATOMID INCHI_DECL IXA_MOL_CreateAtom(IXA_STATUS_HANDLE hStatus,
                                         IXA_MOL_HANDLE    hMolecule)
{
    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_ATOMID_INVALID;

    return MOL_CreateAtom(hStatus, molecule);
}


int INCHI_DECL IXA_MOL_GetNumAtoms(IXA_STATUS_HANDLE hStatus,
                                   IXA_MOL_HANDLE    hMolecule)
{
    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return 0;

    return molecule->atom_count;
}


IXA_ATOMID INCHI_DECL IXA_MOL_GetAtomId(IXA_STATUS_HANDLE hStatus,
                                            IXA_MOL_HANDLE    hMolecule,
                                            int               vAtomIndex)
{
    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_ATOMID_INVALID;

    if (vAtomIndex < 0) return IXA_ATOMID_INVALID;
    if (vAtomIndex >= molecule->atom_count) return IXA_ATOMID_INVALID;
    return MOL_PackAtom(vAtomIndex);
}


int INCHI_DECL IXA_MOL_GetAtomIndex(IXA_STATUS_HANDLE hStatus,
                                    IXA_MOL_HANDLE    hMolecule,
                                    IXA_ATOMID    vAtom)
{
    int atom_index;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return -1;

    if (!MOL_UnpackAtom(hStatus, molecule, vAtom, &atom_index)) return -1;

    return atom_index;
}


int INCHI_DECL IXA_MOL_GetAtomNumBonds(IXA_STATUS_HANDLE hStatus,
                                       IXA_MOL_HANDLE    hMolecule,
                                       IXA_ATOMID    vAtom)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return 0;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return 0;

    return atomptr->bond_count;
}


IXA_BONDID INCHI_DECL IXA_MOL_GetAtomBond(IXA_STATUS_HANDLE hStatus,
                                              IXA_MOL_HANDLE    hMolecule,
                                              IXA_ATOMID    vAtom,
                                              int               vBondIndex)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return 0;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return 0;

    if ((vBondIndex < 0) || (vBondIndex >= atomptr->bond_count))
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Bond index %d is out of range", vBondIndex);
        return IXA_BONDID_INVALID;
    }

    return atomptr->bonds[vBondIndex];
}


void INCHI_DECL IXA_MOL_SetAtomX(IXA_STATUS_HANDLE hStatus,
                                 IXA_MOL_HANDLE    hMolecule,
                                 IXA_ATOMID    vAtom,
                                 double            vX)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return;

    atomptr->x = vX;
}


double INCHI_DECL IXA_MOL_GetAtomX(IXA_STATUS_HANDLE hStatus,
                                   IXA_MOL_HANDLE    hMolecule,
                                   IXA_ATOMID    vAtom)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return 0;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return 0;

    return atomptr->x;
}


void INCHI_DECL IXA_MOL_SetAtomY(IXA_STATUS_HANDLE hStatus,
                                 IXA_MOL_HANDLE    hMolecule,
                                 IXA_ATOMID    vAtom,
                                 double            vY)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return;

    atomptr->y = vY;
}


double INCHI_DECL IXA_MOL_GetAtomY(IXA_STATUS_HANDLE hStatus,
                                   IXA_MOL_HANDLE    hMolecule,
                                   IXA_ATOMID    vAtom)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return 0;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return 0;

    return atomptr->y;
}


void INCHI_DECL IXA_MOL_SetAtomZ(IXA_STATUS_HANDLE hStatus,
                                 IXA_MOL_HANDLE    hMolecule,
                                 IXA_ATOMID    vAtom,
                                 double            vZ)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return;

    atomptr->z = vZ;
}


double INCHI_DECL IXA_MOL_GetAtomZ(IXA_STATUS_HANDLE hStatus,
                                   IXA_MOL_HANDLE    hMolecule,
                                   IXA_ATOMID    vAtom)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return 0;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return 0;

    return atomptr->z;
}


void INCHI_DECL IXA_MOL_SetAtomElement(IXA_STATUS_HANDLE hStatus,
                                       IXA_MOL_HANDLE    hMolecule,
                                       IXA_ATOMID    vAtom,
                                       const char*       pElement)
{
    int            atomic_number;
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return;

    for (atomic_number = 1; atomic_number < (sizeof(Elements) / sizeof(Elements[0])); atomic_number++)
    {
        if (strcmp(pElement, Elements[atomic_number]) == 0)
        {
            atomptr->atomic_number = atomic_number;
            return;
        }
    }
    /*^^^ Fixed original issue with D, T. -IPl */
    if ( !strcmp(pElement, "D") || !strcmp(pElement, "T") )
    {
        atomptr->atomic_number = 1;
        return;
    }

    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Element name %s is not recognised", pElement);
}


const char* INCHI_DECL IXA_MOL_GetAtomElement(IXA_STATUS_HANDLE hStatus,
                                              IXA_MOL_HANDLE    hMolecule,
                                              IXA_ATOMID    vAtom)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return NULL;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return NULL;

    return Elements[atomptr->atomic_number];
}


void INCHI_DECL IXA_MOL_SetAtomAtomicNumber(IXA_STATUS_HANDLE hStatus,
                                            IXA_MOL_HANDLE    hMolecule,
                                            IXA_ATOMID    vAtom,
                                            int               vAtomicNumber)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return;

    if ((vAtomicNumber < 1) || (vAtomicNumber >= sizeof(Elements) / sizeof(Elements[0])))
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Atomic number %d is out of range", vAtomicNumber);
        return;
    }

    atomptr->atomic_number = vAtomicNumber;
}


int INCHI_DECL IXA_MOL_GetAtomAtomicNumber(IXA_STATUS_HANDLE hStatus,
                                           IXA_MOL_HANDLE    hMolecule,
                                           IXA_ATOMID    vAtom)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return 0;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return 0;

    return atomptr->atomic_number;
}


void INCHI_DECL IXA_MOL_SetAtomHydrogens(IXA_STATUS_HANDLE hStatus,
                                         IXA_MOL_HANDLE    hMolecule,
                                         IXA_ATOMID    vAtom,
                                         int               vHydrogenIsotope,
                                         int               vHydrogenCount)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return;

    switch (vHydrogenIsotope)
    {
        case 0:
            if (vHydrogenCount < -1)
            {
                STATUS_PushMessage(hStatus, IXA_STATUS_ERROR,
                    "Implicit hydrogen count may not be negative unless it is -1, meaning deduce from valency");
                return;
            }
            break;
        case 1:
        case 2:
        case 3:
            if (vHydrogenCount < 0)
            {
                STATUS_PushMessage(hStatus, IXA_STATUS_ERROR,
                    "Implicit protium, dueterium and tritium counts may not be negative");
                return;
            }
            break;
        default:
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Hydrogen isotope %d is not recognised", vHydrogenIsotope);
            return;
    }


    atomptr->hydrogens[vHydrogenIsotope] = vHydrogenCount;
}


int INCHI_DECL IXA_MOL_GetAtomHydrogens(IXA_STATUS_HANDLE hStatus,
                                        IXA_MOL_HANDLE    hMolecule,
                                        IXA_ATOMID    vAtom,
                                        int               vHydrogenIsotope)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return 0;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return 0;

    switch (vHydrogenIsotope)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            break;
        default:
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Hydrogen isotope %d is not recognised", vHydrogenIsotope);
            return 0;
    }

    return atomptr->hydrogens[vHydrogenIsotope];
}


void INCHI_DECL IXA_MOL_SetAtomMass(IXA_STATUS_HANDLE hStatus,
                                    IXA_MOL_HANDLE    hMolecule,
                                    IXA_ATOMID    vAtom,
                                    int vMassNumber)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return;

    if (vMassNumber < 0)
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Atom mass may not be negative");
        return;
    }
/*    @@@  Fix 2016-03-01: account for old convention and do not check uncheckable
    TODO: return back a check, also move that Molfile-origin play with
    ISOTOPIC_SHIFT_FLAG to a point just before calling GetINCHI()                */
#if 0
    if ( vMassNumber < ISOTOPIC_SHIFT_FLAG )
    {
        if (vMassNumber > 400) /* The heaviest known element is 294Uuo */
        {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Atom mass is too large");
        return;
        }
    }
#endif
/*    Endfix */

    atomptr->mass = vMassNumber;
}


int INCHI_DECL IXA_MOL_GetAtomMass(IXA_STATUS_HANDLE hStatus,
                                   IXA_MOL_HANDLE    hMolecule,
                                   IXA_ATOMID    vAtom)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return 0;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return 0;

    return atomptr->mass;
}


void INCHI_DECL IXA_MOL_SetAtomRadical(IXA_STATUS_HANDLE   hStatus,
                                       IXA_MOL_HANDLE      hMolecule,
                                       IXA_ATOMID      vAtom,
                                       IXA_ATOM_RADICAL vRadical)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return;

    atomptr->radical = vRadical;
}


IXA_ATOM_RADICAL INCHI_DECL IXA_MOL_GetAtomRadical(IXA_STATUS_HANDLE hStatus,
                                                      IXA_MOL_HANDLE    hMolecule,
                                                      IXA_ATOMID    vAtom)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_ATOM_RADICAL_NONE;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return IXA_ATOM_RADICAL_NONE;

    return atomptr->radical;
}


void INCHI_DECL IXA_MOL_SetAtomCharge(IXA_STATUS_HANDLE hStatus,
                                      IXA_MOL_HANDLE    hMolecule,
                                      IXA_ATOMID    vAtom,
                                      int               vCharge)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return;

    atomptr->charge = vCharge;
}


int INCHI_DECL IXA_MOL_GetAtomCharge(IXA_STATUS_HANDLE hStatus,
                                     IXA_MOL_HANDLE    hMolecule,
                                     IXA_ATOMID    vAtom)
{
    INCHIMOL_ATOM* atomptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return 0;

    atomptr = MOL_GetAtom(hStatus, molecule, vAtom);
    if (!atomptr) return 0;

    return atomptr->charge;
}


IXA_BONDID INCHI_DECL IXA_MOL_CreateBond(IXA_STATUS_HANDLE hStatus,
                                             IXA_MOL_HANDLE    hMolecule,
                                             IXA_ATOMID    vAtom1,
                                             IXA_ATOMID    vAtom2)
{
    IXA_BONDID bond;
    INCHIMOL_BOND*  temp;
    INCHIMOL_ATOM*  atom1ptr;
    INCHIMOL_ATOM*  atom2ptr;
    int bond_index;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_BONDID_INVALID;

    atom1ptr = MOL_GetAtom(hStatus, molecule, vAtom1);
    if (!atom1ptr) return IXA_BONDID_INVALID;

    atom2ptr = MOL_GetAtom(hStatus, molecule, vAtom2);
    if (!atom2ptr) return IXA_BONDID_INVALID;

    if (atom1ptr->bond_count >= sizeof(atom1ptr->bonds) / sizeof(atom1ptr->bonds[0]))
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Atom has too many bonds");
        return IXA_BONDID_INVALID;
    }

    if (atom2ptr->bond_count >= sizeof(atom2ptr->bonds) / sizeof(atom2ptr->bonds[0]))
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Atom has too many bonds");
        return IXA_BONDID_INVALID;
    }

    if (vAtom1 == vAtom2)
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "A bond cannot join an atom to itself");
        return IXA_BONDID_INVALID;
    }

    for (bond_index = 0; bond_index < atom1ptr->bond_count; bond_index++)
    {
        temp = MOL_GetBond(hStatus, molecule, atom1ptr->bonds[bond_index]);
        if (vAtom2 == temp->atom2)
        {
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "A bond already exists between the atoms");
            return IXA_BONDID_INVALID;
        }
    }

    /*
    Reallocate the bonds array to make space for one more bond. Doing things this way means
    that a reallocation occurs every time MOL_CreateBond is called, which is inefficient.
    Alternatives include:
    1. Allocating several bonds at a time so that a reallocations occur only when the pool
       of pre-allocated bonds has been exhausted. This is more efficient but also more
       complicated and wastes memory.
    2. Giving INCHIMOL a function equivalent to std::vector::reserve so that the user can
       pre-allocate exactly the number of bonds that they know they will need. This avoids
       wasting memory but is even more complicated.
    */
    temp = (INCHIMOL_BOND *) inchi_calloc(molecule->bond_count + 1, sizeof(INCHIMOL_BOND));
    if (!temp)
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
        return IXA_BONDID_INVALID;
    }

    memcpy(temp, molecule->bonds, molecule->bond_count * sizeof(INCHIMOL_BOND));
    inchi_free(molecule->bonds);
    molecule->bonds = temp;

    molecule->bonds[molecule->bond_count].atom1            = vAtom1;
    molecule->bonds[molecule->bond_count].atom2            = vAtom2;
    molecule->bonds[molecule->bond_count].type             = IXA_BOND_TYPE_SINGLE;
    molecule->bonds[molecule->bond_count].config           = IXA_DBLBOND_CONFIG_PERCEIVE;
    molecule->bonds[molecule->bond_count].wedge_from_atom1 = IXA_BOND_WEDGE_NONE;
    molecule->bonds[molecule->bond_count].wedge_from_atom2 = IXA_BOND_WEDGE_NONE;

    bond = MOL_PackBond(molecule->bond_count++);
    atom1ptr->bonds[atom1ptr->bond_count] = bond;
    atom1ptr->bond_count++;
    atom2ptr->bonds[atom2ptr->bond_count] = bond;
    atom2ptr->bond_count++;
    return bond;
}


int INCHI_DECL IXA_MOL_GetNumBonds(IXA_STATUS_HANDLE hStatus,
                                   IXA_MOL_HANDLE    hMolecule)
{
    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return 0;

    return molecule->bond_count;
}


IXA_BONDID INCHI_DECL IXA_MOL_GetBondId(IXA_STATUS_HANDLE hStatus,
                                            IXA_MOL_HANDLE    hMolecule,
                                            int               vBondIndex)
{
    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_BONDID_INVALID;

    if (vBondIndex < 0) return IXA_BONDID_INVALID;
    if (vBondIndex >= molecule->bond_count) return IXA_BONDID_INVALID;
    return MOL_PackBond(vBondIndex);
}


int INCHI_DECL IXA_MOL_GetBondIndex(IXA_STATUS_HANDLE hStatus,
                                    IXA_MOL_HANDLE    hMolecule,
                                    IXA_BONDID    vBond)
{
    int bond_index;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return -1;

    if (!MOL_UnpackBond(hStatus, molecule, vBond, &bond_index)) return -1;

    return bond_index;
}


EXPIMP_TEMPLATE INCHI_API IXA_ATOMID INCHI_DECL IXA_MOL_GetBondAtom1(IXA_STATUS_HANDLE hStatus,
                                                                         IXA_MOL_HANDLE    hMolecule,
                                                                         IXA_BONDID    vBond)
{
    INCHIMOL_BOND* bondptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_ATOMID_INVALID;

    bondptr = MOL_GetBond(hStatus, molecule, vBond);
    if (!bondptr) return IXA_ATOMID_INVALID;

    return bondptr->atom1;
}


EXPIMP_TEMPLATE INCHI_API IXA_ATOMID INCHI_DECL IXA_MOL_GetBondAtom2(IXA_STATUS_HANDLE hStatus,
                                                                         IXA_MOL_HANDLE    hMolecule,
                                                                         IXA_BONDID    vBond)
{
    INCHIMOL_BOND* bondptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_ATOMID_INVALID;

    bondptr = MOL_GetBond(hStatus, molecule, vBond);
    if (!bondptr) return IXA_ATOMID_INVALID;

    return bondptr->atom2;
}


void INCHI_DECL IXA_MOL_SetBondType(IXA_STATUS_HANDLE hStatus,
                                    IXA_MOL_HANDLE    hMolecule,
                                    IXA_BONDID    vBond,
                                    IXA_BOND_TYPE  vType)
{
    INCHIMOL_BOND* bondptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    bondptr = MOL_GetBond(hStatus, molecule, vBond);
    if (!bondptr) return;

    bondptr->type = vType;
}


EXPIMP_TEMPLATE INCHI_API IXA_BOND_TYPE INCHI_DECL IXA_MOL_GetBondType(IXA_STATUS_HANDLE hStatus,
                                                                          IXA_MOL_HANDLE    hMolecule,
                                                                          IXA_BONDID    vBond)
{
    INCHIMOL_BOND* bondptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_BOND_TYPE_SINGLE;

    bondptr = MOL_GetBond(hStatus, molecule, vBond);
    if (!bondptr) return IXA_BOND_TYPE_SINGLE;

    return bondptr->type;
}


void INCHI_DECL IXA_MOL_SetBondWedge(IXA_STATUS_HANDLE hStatus,
                                     IXA_MOL_HANDLE    hMolecule,
                                     IXA_BONDID        vBond,
                                     IXA_ATOMID        vRefAtom,
                                     IXA_BOND_WEDGE    vDirection)
{
    INCHIMOL_BOND* bondptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    bondptr = MOL_GetBond(hStatus, molecule, vBond);
    if (!bondptr) return;

    if (vRefAtom == bondptr->atom1)
    {
        bondptr->wedge_from_atom1 = vDirection;
    }
    else if (vRefAtom == bondptr->atom2)
    {
        bondptr->wedge_from_atom2 = vDirection;
    }
    else
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Reference atom is illegal");
        return;
    }
}


IXA_BOND_WEDGE INCHI_DECL IXA_MOL_GetBondWedge(IXA_STATUS_HANDLE hStatus,
                                               IXA_MOL_HANDLE    hMolecule,
                                               IXA_BONDID        vBond,
                                               IXA_ATOMID        vRefAtom)
{
    INCHIMOL_BOND* bondptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_BOND_WEDGE_NONE;

    bondptr = MOL_GetBond(hStatus, molecule, vBond);
    if (!bondptr) return IXA_BOND_WEDGE_NONE;

    if (vRefAtom == bondptr->atom1)
    {
        return bondptr->wedge_from_atom1;
    }
    else if (vRefAtom == bondptr->atom2)
    {
        return bondptr->wedge_from_atom2;
    }
    else
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Reference atom is illegal");
        return IXA_BOND_WEDGE_NONE;
    }
}


void INCHI_DECL IXA_MOL_SetDblBondConfig(IXA_STATUS_HANDLE  hStatus,
                                         IXA_MOL_HANDLE     hMolecule,
                                         IXA_BONDID         vBond,
                                         IXA_DBLBOND_CONFIG vConfig)
{
    INCHIMOL_BOND* bondptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    bondptr = MOL_GetBond(hStatus, molecule, vBond);
    if (!bondptr) return;

    bondptr->config = vConfig;
}


IXA_DBLBOND_CONFIG INCHI_DECL IXA_MOL_GetDblBondConfig(IXA_STATUS_HANDLE hStatus,
                                                          IXA_MOL_HANDLE hMolecule,
                                                          IXA_BONDID     vBond)
{
    INCHIMOL_BOND* bondptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_DBLBOND_CONFIG_PERCEIVE;

    bondptr = MOL_GetBond(hStatus, molecule, vBond);
    if (!bondptr) return IXA_DBLBOND_CONFIG_PERCEIVE;

    return bondptr->config;
}


IXA_BONDID INCHI_DECL IXA_MOL_GetCommonBond(IXA_STATUS_HANDLE hStatus,
                                                IXA_MOL_HANDLE    hMolecule,
                                                IXA_ATOMID    vAtom1,
                                                IXA_ATOMID    vAtom2)
{
    INCHIMOL_ATOM*  atom1ptr;
    INCHIMOL_ATOM*  atom2ptr;
    IXA_BONDID bond1;
    int             bond1_index;
    int             bond2_index;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_BONDID_INVALID;

    atom1ptr = MOL_GetAtom(hStatus, molecule, vAtom1);
    if (!atom1ptr) return IXA_BONDID_INVALID;

    atom2ptr = MOL_GetAtom(hStatus, molecule, vAtom2);
    if (!atom2ptr) return IXA_BONDID_INVALID;

    for (bond1_index = 0; bond1_index < atom1ptr->bond_count; bond1_index++)
    {
        bond1 = atom1ptr->bonds[bond1_index];
        for (bond2_index = 0; bond2_index < atom2ptr->bond_count; bond2_index++)
        {
            if (atom2ptr->bonds[bond2_index] == bond1)
            {
                return bond1;
            }
        }
    }
    return IXA_BONDID_INVALID;
}


IXA_STEREOID INCHI_DECL IXA_MOL_CreateStereoTetrahedron(IXA_STATUS_HANDLE hStatus,
                                                        IXA_MOL_HANDLE    hMolecule,
                                                        IXA_ATOMID        vCentralAtom,
                                                        IXA_ATOMID        vVertex1,
                                                        IXA_ATOMID        vVertex2,
                                                        IXA_ATOMID        vVertex3,
                                                        IXA_ATOMID        vVertex4)
{
    int stereo_index;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_STEREOID_INVALID;

    stereo_index = MOL_CreateStereo(hStatus, molecule);
    if (stereo_index == -1) return IXA_STEREOID_INVALID;

    molecule->stereos[stereo_index].topology       = IXA_STEREO_TOPOLOGY_TETRAHEDRON;
    molecule->stereos[stereo_index].central_entity = vCentralAtom;
    molecule->stereos[stereo_index].vertices[0]    = vVertex1;
    molecule->stereos[stereo_index].vertices[1]    = vVertex2;
    molecule->stereos[stereo_index].vertices[2]    = vVertex3;
    molecule->stereos[stereo_index].vertices[3]    = vVertex4;
    molecule->stereos[stereo_index].parity         = IXA_STEREO_PARITY_NONE;
    return MOL_PackStereo(stereo_index);
}


IXA_STEREOID INCHI_DECL IXA_MOL_CreateStereoRectangle(IXA_STATUS_HANDLE hStatus,
                                                      IXA_MOL_HANDLE    hMolecule,
                                                      IXA_BONDID        vCentralBond,
                                                      IXA_ATOMID        vVertex1,
                                                      IXA_ATOMID        vVertex2,
                                                      IXA_ATOMID        vVertex3,
                                                      IXA_ATOMID        vVertex4)
{
    int stereo_index;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_STEREOID_INVALID;

    stereo_index = MOL_CreateStereo(hStatus, molecule);
    if (stereo_index == -1) return IXA_STEREOID_INVALID;

    molecule->stereos[stereo_index].topology       = IXA_STEREO_TOPOLOGY_RECTANGLE;
    molecule->stereos[stereo_index].central_entity = vCentralBond;
    molecule->stereos[stereo_index].vertices[0]    = vVertex1;
    molecule->stereos[stereo_index].vertices[1]    = vVertex2;
    molecule->stereos[stereo_index].vertices[2]    = vVertex3;
    molecule->stereos[stereo_index].vertices[3]    = vVertex4;
    molecule->stereos[stereo_index].parity         = IXA_STEREO_PARITY_NONE;
    return MOL_PackStereo(stereo_index);
}


IXA_STEREOID INCHI_DECL IXA_MOL_CreateStereoAntiRectangle(IXA_STATUS_HANDLE hStatus,
                                                          IXA_MOL_HANDLE    hMolecule,
                                                          IXA_ATOMID        vCentralAtom,
                                                          IXA_ATOMID        vVertex1,
                                                          IXA_ATOMID        vVertex2,
                                                          IXA_ATOMID        vVertex3,
                                                          IXA_ATOMID        vVertex4)
{
    int stereo_index;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_STEREOID_INVALID;

    stereo_index = MOL_CreateStereo(hStatus, molecule);
    if (stereo_index == -1) return IXA_STEREOID_INVALID;

    molecule->stereos[stereo_index].topology       = IXA_STEREO_TOPOLOGY_ANTIRECTANGLE;
    molecule->stereos[stereo_index].central_entity = vCentralAtom;
    molecule->stereos[stereo_index].vertices[0]    = vVertex1;
    molecule->stereos[stereo_index].vertices[1]    = vVertex2;
    molecule->stereos[stereo_index].vertices[2]    = vVertex3;
    molecule->stereos[stereo_index].vertices[3]    = vVertex4;
    molecule->stereos[stereo_index].parity         = IXA_STEREO_PARITY_NONE;
    return MOL_PackStereo(stereo_index);
}


int INCHI_DECL IXA_MOL_GetNumStereos(IXA_STATUS_HANDLE hStatus,
                                     IXA_MOL_HANDLE    hMolecule)
{
    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return 0;

    return molecule->stereo_count;
}


IXA_STEREOID INCHI_DECL IXA_MOL_GetStereoId(IXA_STATUS_HANDLE hStatus,
                                                IXA_MOL_HANDLE    hMolecule,
                                                int               vStereoIndex)
{
    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_STEREOID_INVALID;

    if (vStereoIndex < 0) return IXA_STEREOID_INVALID;
    if (vStereoIndex >= molecule->stereo_count) return IXA_STEREOID_INVALID;
    return MOL_PackStereo(vStereoIndex);
}


int INCHI_DECL IXA_MOL_GetStereoIndex(IXA_STATUS_HANDLE hStatus,
                                      IXA_MOL_HANDLE    hMolecule,
                                      IXA_STEREOID  vStereo)
{
    int stereo_index;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return -1;

    if (!MOL_UnpackStereo(hStatus, molecule, vStereo, &stereo_index)) return -1;

    return stereo_index;
}


IXA_STEREO_TOPOLOGY INCHI_DECL IXA_MOL_GetStereoTopology(IXA_STATUS_HANDLE hStatus,
                                                         IXA_MOL_HANDLE    hMolecule,
                                                         IXA_STEREOID      vStereo)
{
    INCHIMOL_STEREO* stereoptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_STEREO_TOPOLOGY_INVALID;

    stereoptr = MOL_GetStereo(hStatus, molecule, vStereo);
    if (!stereoptr) return IXA_STEREO_TOPOLOGY_INVALID;

    return stereoptr->topology;
}


IXA_ATOMID INCHI_DECL IXA_MOL_GetStereoCentralAtom(IXA_STATUS_HANDLE hStatus,
                                                   IXA_MOL_HANDLE    hMolecule,
                                                   IXA_STEREOID      vStereo)
{
    INCHIMOL_STEREO* stereoptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_ATOMID_INVALID;

    stereoptr = MOL_GetStereo(hStatus, molecule, vStereo);
    if (!stereoptr) return IXA_ATOMID_INVALID;

    switch (stereoptr->topology)
    {
        case IXA_STEREO_TOPOLOGY_TETRAHEDRON:
        case IXA_STEREO_TOPOLOGY_ANTIRECTANGLE:
            return ((IXA_ATOMID) stereoptr->central_entity);
        case IXA_STEREO_TOPOLOGY_RECTANGLE:
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Stereo centre does not have a central atom");
            return IXA_ATOMID_INVALID;
        default:
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Unexpected stereo topology");
            return IXA_ATOMID_INVALID;
    }
}


IXA_BONDID INCHI_DECL IXA_MOL_GetStereoCentralBond(IXA_STATUS_HANDLE hStatus,
                                                   IXA_MOL_HANDLE    hMolecule,
                                                   IXA_STEREOID      vStereo)
{
    INCHIMOL_STEREO* stereoptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_BONDID_INVALID;

    stereoptr = MOL_GetStereo(hStatus, molecule, vStereo);
    if (!stereoptr) return IXA_BONDID_INVALID;

    switch (stereoptr->topology)
    {
        case IXA_STEREO_TOPOLOGY_TETRAHEDRON:
        case IXA_STEREO_TOPOLOGY_ANTIRECTANGLE:
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Stereo centre does not have a central bond");
            return IXA_BONDID_INVALID;
        case IXA_STEREO_TOPOLOGY_RECTANGLE:
            return ((IXA_BONDID) stereoptr->central_entity);
        default:
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Unexpected stereo topology");
            return IXA_BONDID_INVALID;
    }
}


int INCHI_DECL IXA_MOL_GetStereoNumVertices(IXA_STATUS_HANDLE hStatus,
                                            IXA_MOL_HANDLE    hMolecule,
                                            IXA_STEREOID      vStereo)
{
    INCHIMOL_STEREO* stereoptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return 0;

    stereoptr = MOL_GetStereo(hStatus, molecule, vStereo);
    if (!stereoptr) return 0;

    return GetVertexCount(hStatus, stereoptr->topology);
}


IXA_ATOMID INCHI_DECL IXA_MOL_GetStereoVertex(IXA_STATUS_HANDLE hStatus,
                                              IXA_MOL_HANDLE    hMolecule,
                                              IXA_STEREOID      vStereo,
                                              int               vVertexIndex)
{
    int              vertex_count;
    INCHIMOL_STEREO* stereoptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_ATOMID_INVALID;

    stereoptr = MOL_GetStereo(hStatus, molecule, vStereo);
    if (!stereoptr) return IXA_ATOMID_INVALID;

    vertex_count = GetVertexCount(hStatus, stereoptr->topology);
    if (IXA_STATUS_HasError(hStatus)) return IXA_ATOMID_INVALID;

    if ((vVertexIndex < 0) || (vVertexIndex >= vertex_count))
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Vertex index is out of range");
        return IXA_ATOMID_INVALID;
    }

    return stereoptr->vertices[vVertexIndex];
}


INCHI_API void INCHI_DECL IXA_MOL_SetStereoParity(IXA_STATUS_HANDLE hStatus,
                                                  IXA_MOL_HANDLE    hMolecule,
                                                  IXA_STEREOID      vStereo,
                                                  IXA_STEREO_PARITY vParity)
{
    INCHIMOL_STEREO* stereoptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return;

    stereoptr = MOL_GetStereo(hStatus, molecule, vStereo);
    if (!stereoptr) return;

    stereoptr->parity = vParity;
}


IXA_STEREO_PARITY INCHI_DECL IXA_MOL_GetStereoParity(IXA_STATUS_HANDLE hStatus,
                                                     IXA_MOL_HANDLE    hMolecule,
                                                     IXA_STEREOID      vStereo)
{
    INCHIMOL_STEREO* stereoptr;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return IXA_STEREO_PARITY_NONE;

    stereoptr = MOL_GetStereo(hStatus, molecule, vStereo);
    if (!stereoptr) return IXA_STEREO_PARITY_NONE;

    return stereoptr->parity;
}


/*
    Extended input supporting v. 1.05 extensions: V3000; polymers
    This is a later addition to native IXA intended at this time
    just to be a carrier which transports extension data to GetINCHI()
*/

#include "../../../../INCHI_BASE/src/mol_fmt.h"

int  IXA_MOL_SetExtMoldataByMolfileExtInput( IXA_STATUS_HANDLE hStatus,
                                             IXA_MOL_HANDLE    hMolecule,
                                             MOL_FMT_DATA* mfdata)
{
    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule) return 0;

    {
        int    k, m, err = 0;
        int nsgroups = mfdata->ctab.sgroups.used;

        /* Polymers */
        if ( nsgroups > 0 )
        {
            /* Prepare OrigAtDataPolymer container */
            molecule->polymer = (INCHIMOL_POLYMER *) inchi_calloc( 1, sizeof(INCHIMOL_POLYMER) );
            if ( !molecule->polymer )
            {
                STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Polymer data error, no memory ");
                return IXA_EXT_POLYMER_INVALID;
            }

            molecule->polymer->units = (INCHIMOL_SGROUP**) inchi_calloc( nsgroups, sizeof(molecule->polymer->units[0]) );
            if ( !molecule->polymer->units )
            {
                STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Polymer data error, no memory ");
                return IXA_EXT_POLYMER_INVALID;
            }
            memset( molecule->polymer->units, 0, sizeof( *(molecule->polymer->units) ) );

            molecule->polymer->n = nsgroups;

            for (k=0; k<nsgroups; k++ )
            {
                int q=0;
                MOL_FMT_SGROUP*     groupk    =    mfdata->ctab.sgroups.group[k];

                INCHIMOL_SGROUP*     unitk    =    molecule->polymer->units[k] = (INCHIMOL_SGROUP*) inchi_calloc( 1, sizeof(INCHIMOL_SGROUP) );
                if (!unitk )
                {
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Polymer data error, no memory ");
                    return IXA_EXT_POLYMER_INVALID;
                }
                memset( unitk, 0, sizeof( *unitk ) );

                unitk->id                =    groupk->id;
                unitk->type                =    groupk->type;
                unitk->subtype            =    groupk->subtype;
                unitk->conn                =    groupk->conn;
                unitk->label            =    groupk->label;

                for (q=0; q<4; q++)
                {
                    unitk->xbr1[q] = groupk->xbr1[q];
                    unitk->xbr2[q] = groupk->xbr2[q];
                }
                strcpy( unitk->smt, groupk->smt );
                unitk->na = groupk->alist.used;
                unitk->alist = (int *) inchi_calloc( unitk->na, sizeof(int) );
                if (!unitk->alist )
                {
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Polymer data error, no memory");
                    return IXA_EXT_POLYMER_INVALID;
                }
                for (m=0; m<unitk->na; m++)
                {
                    unitk->alist[m] = groupk->alist.item[m];
                }
                unitk->nb = groupk->blist.used;
                if ( unitk->nb > 0 )
                {
                    unitk->blist = (int *) inchi_calloc( 2*unitk->nb, sizeof(int) );
                    if (!unitk->blist )
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Polymer data error, no memory");
                        return IXA_EXT_POLYMER_INVALID;
                    }
                    for (m=0; m < groupk->blist.used; m++)
                    {
                        int ib, ia1, ia2;
                        ib = groupk->blist.item[m];
                        if ( ib<1 || ib>mfdata->ctab.n_bonds )
                        {
                            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Polymer unit refers to invalid bond");
                            return IXA_EXT_POLYMER_INVALID;
                        }
                        ia1 = mfdata->ctab.bonds[ib-1].atnum1;
                        ia2 = mfdata->ctab.bonds[ib-1].atnum2;
                        unitk->blist[2*m]    = ia1;
                        unitk->blist[2*m+1]    = ia2;
                        if ( !strcmp(mfdata->ctab.atoms[ia1-1].symbol,"H") ||
                             !strcmp(mfdata->ctab.atoms[ia2-1].symbol,"H")  )
                        {
                            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Hydrogen as polymer end group is not supported");
                            return IXA_EXT_POLYMER_INVALID;
                        }
                    }
                }
                else
                    unitk->blist = NULL;
            }
        }

        /* V3000 Extensions */
        if ( mfdata->ctab.v3000 )
        {
            int m, k, nn;
            MOL_FMT_v3000 *mpv        = mfdata->ctab.v3000;

            molecule->v3000    = (INCHIMOL_V3000 *) inchi_calloc( 1, sizeof(INCHIMOL_V3000) );
            if ( !molecule->v3000 )
            {
                STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                return IXA_EXT_V3000_INVALID;
            }
            memset( molecule->v3000, 0, sizeof( *molecule->v3000 ) );


            molecule->v3000->n_collections        = mpv->n_collections;
            molecule->v3000->n_haptic_bonds        = mpv->n_haptic_bonds;
            molecule->v3000->n_non_haptic_bonds    = mpv->n_non_haptic_bonds;
            molecule->v3000->n_sgroups            = mpv->n_sgroups;
            molecule->v3000->n_non_star_atoms    = mpv->n_non_star_atoms;
            molecule->v3000->n_star_atoms        = mpv->n_star_atoms;
            molecule->v3000->n_steabs            = mpv->n_steabs;
            molecule->v3000->n_sterac            = mpv->n_sterac;
            molecule->v3000->n_sterel            = mpv->n_sterel;
            molecule->v3000->n_3d_constraints    = mpv->n_3d_constraints;

            if ( mpv->atom_index_orig )
            {
                molecule->v3000->atom_index_orig = (int *) inchi_calloc( mfdata->ctab.n_atoms, sizeof(int) );
                if ( NULL==molecule->v3000->atom_index_orig )
                {
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                    return IXA_EXT_V3000_INVALID;
                }
                memcpy( molecule->v3000->atom_index_orig, mpv->atom_index_orig, mfdata->ctab.n_atoms);
            }
            if ( mpv->atom_index_fin )
            {
                molecule->v3000->atom_index_fin = (int *) inchi_calloc( mfdata->ctab.n_atoms, sizeof(int) );
                if ( NULL==molecule->v3000->atom_index_fin )
                {
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                    return IXA_EXT_V3000_INVALID;
                }
                memcpy( molecule->v3000->atom_index_fin, mpv->atom_index_fin, mfdata->ctab.n_atoms);
            }
            if ( mpv->n_haptic_bonds && mpv->haptic_bonds )
            {
                molecule->v3000->lists_haptic_bonds = (int **) calloc( mpv->n_haptic_bonds, sizeof (int*) );
                if ( NULL==molecule->v3000->lists_haptic_bonds )
                {
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                    return IXA_EXT_V3000_INVALID;
                }
                for (m=0; m<mpv->n_haptic_bonds; m++)
                {
                    int *lst=NULL;
                    int *mol_lst = mpv->haptic_bonds->lists[m];
                    nn = mol_lst[2] + 3;
                    lst = molecule->v3000->lists_haptic_bonds[m] = (int *) calloc( nn, sizeof (int) );
                    if ( NULL==lst )
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                        return IXA_EXT_V3000_INVALID;
                    }
                    for (k=0; k<nn; k++)
                        lst[k] = mol_lst[k];
                }
            }
            if ( mpv->n_steabs && mpv->steabs )
            {
                molecule->v3000->lists_steabs = (int **) calloc( mpv->n_steabs, sizeof (int*) );
                if ( NULL==molecule->v3000->lists_steabs )
                    { err = 16; goto exitf;  }
                for (m=0; m<mpv->n_steabs; m++)
                {
                    int *lst=NULL;
                    int *mol_lst = mpv->steabs->lists[m];
                    nn = mol_lst[1] + 2;
                    lst = molecule->v3000->lists_steabs[m] = (int *) calloc( nn, sizeof (int) );
                    if ( NULL==lst )
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                        return IXA_EXT_V3000_INVALID;
                    }
                    for (k=0; k<nn; k++)
                        lst[k] = mol_lst[k];
                }
            }
            if ( mpv->n_sterac && mpv->sterac )
            {
                molecule->v3000->lists_sterac = (int **) calloc( mpv->n_sterac, sizeof (int*) );
                if ( NULL==molecule->v3000->lists_sterac )
                    { err = 16; goto exitf;  }
                for (m=0; m<mpv->n_sterac; m++)
                {
                    int *lst=NULL;
                    int *mol_lst = mpv->sterac->lists[m];
                    nn = mol_lst[1] + 2;
                    lst = molecule->v3000->lists_sterac[m] = (int *) calloc( nn, sizeof (int) );
                    if ( NULL==lst )
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                        return IXA_EXT_V3000_INVALID;
                    }
                    for (k=0; k<nn; k++)
                        lst[k] = mol_lst[k];
                }
            }
            if ( mpv->n_sterel && mpv->sterel )
            {
                molecule->v3000->lists_sterel = (int **) calloc( mpv->n_sterel, sizeof (int*) );
                if ( NULL==molecule->v3000->lists_sterel )
                    { err = 16; goto exitf;  }
                for (m=0; m<mpv->n_sterel; m++)
                {
                    int *lst=NULL;
                    int *mol_lst = mpv->sterel->lists[m];
                    nn = mol_lst[1] + 2;
                    lst = molecule->v3000->lists_sterel[m] = (int *) calloc( nn, sizeof (int) );
                    if ( NULL==lst )
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                        return IXA_EXT_V3000_INVALID;
                    }
                    for (k=0; k<nn; k++)
                        lst[k] = mol_lst[k];
                }
            }
        }


    exitf:
        if ( err )
            IXA_MOL_ClearExtMolData( molecule->polymer, molecule->v3000 );

        return err;
    }
}


void IXA_MOL_ClearExtMolData( INCHIMOL_POLYMER *pd, INCHIMOL_V3000 *v3k )
{
    int k;


    /* Polymers */
    if ( pd )
    {
        if ( pd->n && pd->units )
        {
            int k;
            for (k=0; k<pd->n; k++)
            {
                if ( pd->units[k] )
                {
                    if ( pd->units[k]->alist )
                        { inchi_free( pd->units[k]->alist );  pd->units[k]->alist = NULL; }
                    if ( pd->units[k]->blist )
                        { inchi_free( pd->units[k]->blist );  pd->units[k]->blist = NULL; }
                }
                inchi_free( pd->units[k] );
            }
            inchi_free( pd->units );
            pd->units = NULL;
            pd->n = 0;
        }
        free( pd );
        pd = NULL;
    }

    /* V3000 Extensions */
    if ( v3k )
    {
        if ( v3k->atom_index_orig )
        {
            inchi_free( v3k->atom_index_orig );
            v3k->atom_index_orig = NULL;
        }
        if ( v3k->atom_index_fin )
        {
            inchi_free( v3k->atom_index_fin );
            v3k->atom_index_fin = NULL;
        }
        if ( v3k->n_haptic_bonds && v3k->lists_haptic_bonds )
        {
            for (k=0; k<v3k->n_haptic_bonds; k++)
                if ( v3k->lists_haptic_bonds[k] )
                {
                    inchi_free( v3k->lists_haptic_bonds[k] );
                    v3k->lists_haptic_bonds[k] = NULL;
                }
            inchi_free( v3k->lists_haptic_bonds );
            v3k->lists_haptic_bonds = NULL;
        }
        if ( v3k->n_steabs && v3k->lists_steabs )
        {
            for (k=0; k<v3k->n_steabs; k++)
                if ( v3k->lists_steabs[k] )
                {
                    inchi_free( v3k->lists_steabs[k] );
                    v3k->lists_steabs[k] = NULL;
                }
            inchi_free( v3k->lists_steabs );
            v3k->lists_steabs = NULL;
        }
        if ( v3k->n_sterel && v3k->lists_sterel )
        {
            for (k=0; k<v3k->n_sterel; k++)
                if ( v3k->lists_sterel[k] )
                {
                    inchi_free( v3k->lists_sterel[k] );
                    v3k->lists_sterel[k] = NULL;
                }
            inchi_free( v3k->lists_sterel );
            v3k->lists_sterel = NULL;
        }
        if ( v3k->n_sterac && v3k->lists_sterac )
        {
            for (k=0; k<v3k->n_sterac; k++)
                if ( v3k->lists_sterac[k] )
                {
                    inchi_free( v3k->lists_sterac[k] );
                    v3k->lists_sterac[k] = NULL;
                }
            inchi_free( v3k->lists_sterac );
            v3k->lists_sterac = NULL;
        }
        /*memset( v3k, 0, sizeof( v3k ) );*/
        free( v3k );
        v3k = NULL;
    }

    return;
}


int  IXA_MOL_SetExtMolDataByInChIExtInput( IXA_STATUS_HANDLE hStatus,
                                           IXA_MOL_HANDLE    hMolecule,
                                           inchi_Output_Polymer    *polymer,
                                           inchi_Output_V3000        *v3000,
                                           int nat )
{
int    n, k, m, err = 0;

    INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
    if (!molecule)
        return 0;

    if ( polymer && polymer->n )
    {
        n = polymer->n;
        nat = molecule->atom_count;

        /* Polymers */
        if ( n > 0 )
        {
            /* Prepare OrigAtDataPolymer container */
            molecule->polymer = (INCHIMOL_POLYMER *) inchi_calloc( 1, sizeof(INCHIMOL_POLYMER) );
            if ( !molecule->polymer )
            {
                STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Polymer data error, no memory ");
                return IXA_EXT_POLYMER_INVALID;
            }

            molecule->polymer->units = (INCHIMOL_SGROUP**) inchi_calloc( n, sizeof(molecule->polymer->units[0]) );
            if ( !molecule->polymer->units )
            {
                STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Polymer data error, no memory ");
                return IXA_EXT_POLYMER_INVALID;
            }
            memset( molecule->polymer->units, 0, sizeof( *(molecule->polymer->units) ) );

            molecule->polymer->n = n;

            for (k=0; k<n; k++ )
            {
                int q=0;
                inchi_Output_PolymerUnit        *groupk    =    polymer->units[k];
                INCHIMOL_SGROUP*     unitk    =    molecule->polymer->units[k] = (INCHIMOL_SGROUP*) inchi_calloc( 1, sizeof(INCHIMOL_SGROUP) );
                if (!unitk )
                {
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Polymer data error, no memory ");
                    return IXA_EXT_POLYMER_INVALID;
                }
                memset( unitk, 0, sizeof( *unitk ) );

                unitk->id                =    groupk->id;
                unitk->type                =    groupk->type;
                unitk->subtype            =    groupk->subtype;
                unitk->conn                =    groupk->conn;
                unitk->label            =    groupk->label;

                for (q=0; q<4; q++)
                {
                    unitk->xbr1[q] = groupk->xbr1[q];
                    unitk->xbr2[q] = groupk->xbr2[q];
                }
                strcpy( unitk->smt, groupk->smt );
                unitk->na = groupk->na;
                unitk->alist = (int *) inchi_calloc( unitk->na, sizeof(int) );
                if (!unitk->alist )
                {
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Polymer data error, no memory");
                    return IXA_EXT_POLYMER_INVALID;
                }
                for (m=0; m<unitk->na; m++)
                    unitk->alist[m] = groupk->alist[m];
                unitk->nb = groupk->nb;
                if ( unitk->nb > 0 )
                {
                    unitk->blist = (int *) inchi_calloc( 2*unitk->nb, sizeof(int) );
                    if (!unitk->blist )
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Polymer data error, no memory");
                        return IXA_EXT_POLYMER_INVALID;
                    }
                    for (m=0; m < 2*groupk->nb; m++)
                        unitk->blist[m]    = groupk->blist[m];
                }
                else
                    unitk->blist = NULL;
            }
        }
    }

    /* V3000 Extensions */
    if ( v3000 )
    {
        int m, k, nn;

        molecule->v3000    = (INCHIMOL_V3000 *) inchi_calloc( 1, sizeof(INCHIMOL_V3000) );

        if ( !molecule->v3000 )
        {
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
            return IXA_EXT_V3000_INVALID;
        }
        memset( molecule->v3000, 0, sizeof( *molecule->v3000 ) );

        molecule->v3000->n_collections        = v3000->n_collections;
        molecule->v3000->n_haptic_bonds        = v3000->n_haptic_bonds;
        molecule->v3000->n_non_haptic_bonds    = v3000->n_non_haptic_bonds;
        molecule->v3000->n_sgroups            = v3000->n_sgroups;
        molecule->v3000->n_non_star_atoms    = v3000->n_non_star_atoms;
        molecule->v3000->n_star_atoms        = v3000->n_star_atoms;
        molecule->v3000->n_steabs            = v3000->n_steabs;
        molecule->v3000->n_sterac            = v3000->n_sterac;
        molecule->v3000->n_sterel            = v3000->n_sterel;
        molecule->v3000->n_3d_constraints    = v3000->n_3d_constraints;

        if ( v3000->atom_index_orig )
        {
            molecule->v3000->atom_index_orig = (int *) inchi_calloc( nat, sizeof(int) );
            if ( NULL==molecule->v3000->atom_index_orig )
            {
                STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                return IXA_EXT_V3000_INVALID;
            }
            memcpy( molecule->v3000->atom_index_orig, v3000->atom_index_orig, nat);
        }
        if ( v3000->atom_index_fin )
        {
            molecule->v3000->atom_index_fin = (int *) inchi_calloc( nat, sizeof(int) );
            if ( NULL==molecule->v3000->atom_index_fin )
            {
                STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                return IXA_EXT_V3000_INVALID;
            }
            memcpy( molecule->v3000->atom_index_fin, v3000->atom_index_fin, nat);
        }
        if ( v3000->n_haptic_bonds && v3000->lists_haptic_bonds )
        {
            molecule->v3000->lists_haptic_bonds = (int **) calloc( v3000->n_haptic_bonds, sizeof (int*) );
            if ( NULL==molecule->v3000->lists_haptic_bonds )
            {
                STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                return IXA_EXT_V3000_INVALID;
            }
            for (m=0; m<v3000->n_haptic_bonds; m++)
            {
                int *lst=NULL;
                int *mol_lst = v3000->lists_haptic_bonds[m];
                nn = mol_lst[2] + 3;
                lst = molecule->v3000->lists_haptic_bonds[m] = (int *) calloc( nn, sizeof (int) );
                if ( NULL==lst )
                {
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                    return IXA_EXT_V3000_INVALID;
                }
                for (k=0; k<nn; k++)
                    lst[k] = mol_lst[k];
            }
        }
        if ( v3000->n_steabs && v3000->lists_steabs )
        {
            molecule->v3000->lists_steabs = (int **) calloc( v3000->n_steabs, sizeof (int*) );
            if ( NULL==molecule->v3000->lists_steabs )
                { err = 16; goto exitf;  }
            for (m=0; m<v3000->n_steabs; m++)
            {
                int *lst=NULL;
                int *mol_lst = v3000->lists_steabs[m];
                nn = mol_lst[1] + 2;
                lst = molecule->v3000->lists_steabs[m] = (int *) calloc( nn, sizeof (int) );
                if ( NULL==lst )
                {
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                    return IXA_EXT_V3000_INVALID;
                }
                for (k=0; k<nn; k++)
                    lst[k] = mol_lst[k];
            }
        }
        if ( v3000->n_sterac && v3000->lists_sterac )
        {
            molecule->v3000->lists_sterac = (int **) calloc( v3000->n_sterac, sizeof (int*) );
            if ( NULL==molecule->v3000->lists_sterac )
                { err = 16; goto exitf;  }
            for (m=0; m<v3000->n_sterac; m++)
            {
                int *lst=NULL;
                int *mol_lst = v3000->lists_sterac[m];
                nn = mol_lst[1] + 2;
                lst = molecule->v3000->lists_sterac[m] = (int *) calloc( nn, sizeof (int) );
                if ( NULL==lst )
                {
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                    return IXA_EXT_V3000_INVALID;
                }
                for (k=0; k<nn; k++)
                    lst[k] = mol_lst[k];
            }
        }
        if ( v3000->n_sterel && v3000->lists_sterel )
        {
            molecule->v3000->lists_sterel = (int **) calloc( v3000->n_sterel, sizeof (int*) );
            if ( NULL==molecule->v3000->lists_sterel )
                { err = 16; goto exitf;  }
            for (m=0; m<v3000->n_sterel; m++)
            {
                int *lst=NULL;
                int *mol_lst = v3000->lists_sterel[m];
                nn = mol_lst[1] + 2;
                lst = molecule->v3000->lists_sterel[m] = (int *) calloc( nn, sizeof (int) );
                if ( NULL==lst )
                {
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "V3000 data error, no memory");
                    return IXA_EXT_V3000_INVALID;
                }
                for (k=0; k<nn; k++)
                    lst[k] = mol_lst[k];
            }
        }
    }


    exitf:
        if ( err )
            IXA_MOL_ClearExtMolData( molecule->polymer, molecule->v3000 );

        return err;
}
