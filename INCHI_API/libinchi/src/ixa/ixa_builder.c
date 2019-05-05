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


#include "../../../../INCHI_BASE/src/mode.h"
#include "../../../../INCHI_BASE/src/inchi_api.h"
#include "../../../../INCHI_BASE/src/inpdef.h"
#include "ixa_status.h"
#include "ixa_mol.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>


#ifdef _WIN32
#define OPTION_PREFIX  "/"
#else
#define OPTION_PREFIX  "-"
#endif


typedef struct
{
    AT_NUM                    num_atoms;        /* number of atoms in the structure                    */
    inchi_Atom*                atom;            /* array of num_atoms elements                        */

    AT_NUM                    num_stereo0D;    /* number of 0D stereo elements                        */
    inchi_Stereo0D*            stereo0D;        /* array of num_stereo0D 0D stereo elements or NULL    */

    int                        chiral;            /* 1 if the molecule is chiral, 0 if it is not.        */

    inchi_Input_Polymer    *polymer;        /* later extension                                    */
    inchi_Input_V3000        *v3000;            /* later extension                                    */
} BuilderMolecule;


typedef struct
{
    BuilderMolecule molecule;

    int                           option_Wnumber;
    IXA_INCHIBUILDER_STEREOOPTION option_stereo;
    IXA_BOOL                      option_NEWPSOFF;
    IXA_BOOL                      option_DoNotAddH;
    IXA_BOOL                      option_SUU;
    IXA_BOOL                      option_SLUUD;
    IXA_BOOL                      option_FixedH;
    IXA_BOOL                      option_RecMet;
    IXA_BOOL                      option_KET;
    IXA_BOOL                      option_15T;
    IXA_BOOL                      option_AuxNone;
    IXA_BOOL                      option_WarnOnEmptyStructure;
    IXA_BOOL                      option_LargeMolecules;
    IXA_BOOL                      option_SaveOpt;

    char* inchi;
    char* auxinfo;
    char* log;

    IXA_BOOL dirty;
} INCHIBUILDER;



static int GetSingleStereoCode(IXA_STATUS_HANDLE hStatus,
                               IXA_BOND_WEDGE    direction,
                               IXA_BOND_WEDGE    reverse_direction)
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
                default:
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Illegal bond detected");
                    return INCHI_BOND_STEREO_NONE;
            }

        case IXA_BOND_WEDGE_UP:
            return INCHI_BOND_STEREO_SINGLE_1UP;
        case IXA_BOND_WEDGE_DOWN:
            return INCHI_BOND_STEREO_SINGLE_1DOWN;
        case IXA_BOND_WEDGE_EITHER:
            return INCHI_BOND_STEREO_SINGLE_1EITHER;
        default:
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Illegal bond detected");
            return INCHI_BOND_STEREO_NONE;
    }
}


static int GetDoubleStereoCode(IXA_STATUS_HANDLE     hStatus,
                               IXA_DBLBOND_CONFIG vConfig)
{
    switch (vConfig)
    {
        case IXA_DBLBOND_CONFIG_PERCEIVE:
            return INCHI_BOND_STEREO_NONE;
        case IXA_DBLBOND_CONFIG_EITHER:
            return INCHI_BOND_STEREO_DOUBLE_EITHER;
        default:
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Illegal bond detected");
            return INCHI_BOND_STEREO_NONE;
    }
}


static INCHIBUILDER* BUILDER_Unpack(IXA_STATUS_HANDLE       hStatus,
                                    IXA_INCHIBUILDER_HANDLE hBuilder)
{
    if (!hBuilder)
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Illegal builder handle detected");
        return NULL;
    }

    return (INCHIBUILDER*)hBuilder;
}


static IXA_INCHIBUILDER_HANDLE BUILDER_Pack(INCHIBUILDER* pBuilder)
{
    return (IXA_INCHIBUILDER_HANDLE)pBuilder;
}


static void TranslateTetrahedralVertex(IXA_STATUS_HANDLE hStatus,
                                       IXA_MOL_HANDLE    hMolecule,
                                       IXA_STEREOID      vSourceStereo,
                                       inchi_Stereo0D*   pTargetStereo,
                                       int               vVertexIndex)
{
    IXA_ATOMID vertex = IXA_MOL_GetStereoVertex(hStatus, hMolecule, vSourceStereo, vVertexIndex);
    if (IXA_STATUS_HasError(hStatus)) return;
    if (vertex == IXA_ATOMID_IMPLICIT_H)
    {
        pTargetStereo->neighbor[vVertexIndex] = pTargetStereo->central_atom;
    }
    else
    {
        pTargetStereo->neighbor[vVertexIndex] = IXA_MOL_GetAtomIndex(hStatus, hMolecule, vertex);
        if (IXA_STATUS_HasError(hStatus)) return;
    }
}


static void ExtendAllene(IXA_STATUS_HANDLE hStatus,
                         IXA_MOL_HANDLE    hMolecule,
                         IXA_ATOMID        vCentralAtom,
                         IXA_ATOMID*       pAtom1,
                         IXA_ATOMID*       pAtom2)
{
    int              bond_count;
    IXA_BONDID   bond;
    IXA_BOND_TYPE bond_type;

    bond_count = IXA_MOL_GetAtomNumBonds(hStatus, hMolecule, vCentralAtom);
    if (IXA_STATUS_HasError(hStatus)) return;
    if (bond_count != 2)
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Stereo centre is not allene");
        return;
    }

    bond = IXA_MOL_GetAtomBond(hStatus, hMolecule, vCentralAtom, 0);
    if (IXA_STATUS_HasError(hStatus)) return;
    bond_type = IXA_MOL_GetBondType(hStatus, hMolecule, bond);
    if (IXA_STATUS_HasError(hStatus)) return;
    if (bond_type != IXA_BOND_TYPE_DOUBLE)
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR,
                            "Internal bonds of allenes must be double");
        return;
    }
    *pAtom1 = MOL_GetBondOtherAtom(hStatus, hMolecule, bond, vCentralAtom);
    if (IXA_STATUS_HasError(hStatus)) return;

    bond = IXA_MOL_GetAtomBond(hStatus, hMolecule, vCentralAtom, 1);
    if (IXA_STATUS_HasError(hStatus)) return;
    bond_type = IXA_MOL_GetBondType(hStatus, hMolecule, bond);
    if (IXA_STATUS_HasError(hStatus)) return;
    if (bond_type != IXA_BOND_TYPE_DOUBLE)
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR,
                            "Internal bonds of allenes must be double");
        return;
    }
    *pAtom2 = MOL_GetBondOtherAtom(hStatus, hMolecule, bond, vCentralAtom);
    if (IXA_STATUS_HasError(hStatus)) return;
}


static IXA_ATOMID ExtendCumulene(IXA_STATUS_HANDLE hStatus,
                                 IXA_MOL_HANDLE    hMolecule,
                                 IXA_BONDID        vBond,
                                 IXA_ATOMID        vAtom)
{
    int              bond_count;
    IXA_BONDID   bond;
    IXA_BOND_TYPE bond_type;
    IXA_ATOMID   atom;

    bond_count = IXA_MOL_GetAtomNumBonds(hStatus, hMolecule, vAtom);
    if (IXA_STATUS_HasError(hStatus)) return IXA_ATOMID_INVALID;
    if (bond_count != 2)
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Stereo centre is neither olefin nor cumulene");
        return IXA_ATOMID_INVALID;
    }
    bond = IXA_MOL_GetAtomBond(hStatus, hMolecule, vAtom, 0);
    if (IXA_STATUS_HasError(hStatus)) return IXA_ATOMID_INVALID;
    if (bond == vBond)
    {
        bond = IXA_MOL_GetAtomBond(hStatus, hMolecule, vAtom, 1);
        if (IXA_STATUS_HasError(hStatus)) return IXA_ATOMID_INVALID;
    }
    bond_type = IXA_MOL_GetBondType(hStatus, hMolecule, bond);
    if (IXA_STATUS_HasError(hStatus)) return IXA_ATOMID_INVALID;
    if (bond_type != IXA_BOND_TYPE_DOUBLE)
    {
        STATUS_PushMessage( hStatus, IXA_STATUS_ERROR,
                            "Internal bonds of cumulenes must be double");
        return IXA_ATOMID_INVALID;
    }
    atom = MOL_GetBondOtherAtom(hStatus, hMolecule, bond, vAtom);
    if (IXA_STATUS_HasError(hStatus)) return IXA_ATOMID_INVALID;
    return atom;
}


static IXA_BOOL IsRectangularVertex(IXA_STATUS_HANDLE hStatus,
                                    IXA_MOL_HANDLE    hMolecule,
                                    IXA_ATOMID    vVertex,
                                    IXA_ATOMID    vInternal)
{
    IXA_BONDID bond;

    if (vVertex != IXA_ATOMID_IMPLICIT_H)
    {
        bond = IXA_MOL_GetCommonBond(hStatus, hMolecule, vVertex, vInternal);
        if (IXA_STATUS_HasError(hStatus)) return IXA_FALSE;
        if (bond == IXA_BONDID_INVALID)
        {
            return IXA_FALSE;
        }
        else
        {
            return IXA_TRUE;
        }
    }
    else
    {
        return IXA_TRUE;
    }
}


static IXA_BOOL IsRectOrAntiRectCentre(IXA_STATUS_HANDLE hStatus,
                                       IXA_MOL_HANDLE    hMolecule,
                                       IXA_ATOMID    vVertex1a,
                                       IXA_ATOMID    vVertex1b,
                                       IXA_ATOMID    vInternal1,
                                       IXA_ATOMID    vVertex2a,
                                       IXA_ATOMID    vVertex2b,
                                       IXA_ATOMID    vInternal2)
{
    if (!IsRectangularVertex(hStatus, hMolecule, vVertex1a, vInternal1)) return IXA_FALSE;
    if (!IsRectangularVertex(hStatus, hMolecule, vVertex1b, vInternal1)) return IXA_FALSE;
    if (!IsRectangularVertex(hStatus, hMolecule, vVertex2a, vInternal2)) return IXA_FALSE;
    if (!IsRectangularVertex(hStatus, hMolecule, vVertex2b, vInternal2)) return IXA_FALSE;
    return IXA_TRUE;
}


static void ClearMolecule(BuilderMolecule* pMolecule)
{
    inchi_free(pMolecule->atom);
    pMolecule->atom = NULL;
    pMolecule->num_atoms = 0;

    inchi_free(pMolecule->stereo0D);
    pMolecule->stereo0D = NULL;
    pMolecule->num_stereo0D = 0;

    pMolecule->chiral = 0;

    FreeInChIExtInput( pMolecule->polymer,  pMolecule->v3000 );
    pMolecule->polymer    = NULL;
    pMolecule->v3000    = NULL;
}


static void AppendOption(char*       pString,
                         const char* pOption)
{
    if (pString[0] != '\0')
    {
        strcat(pString, " ");
    }
    strcat(pString, pOption);
}


static void BUILDER_ClearOptions(INCHIBUILDER* pBuilder)
{
    pBuilder->option_NEWPSOFF             = IXA_FALSE;
    pBuilder->option_DoNotAddH            = IXA_FALSE;
    pBuilder->option_stereo               = IXA_INCHIBUILDER_STEREOOPTION_SAbs;
    pBuilder->option_SUU                  = IXA_FALSE;
    pBuilder->option_SLUUD                = IXA_FALSE;
    pBuilder->option_FixedH               = IXA_FALSE;
    pBuilder->option_RecMet               = IXA_FALSE;
    pBuilder->option_KET                  = IXA_FALSE;
    pBuilder->option_15T                  = IXA_FALSE;
    pBuilder->option_AuxNone              = IXA_FALSE;
    pBuilder->option_Wnumber              = 0; /* i.e. unlimited time */
    pBuilder->option_WarnOnEmptyStructure = IXA_FALSE;
    pBuilder->option_LargeMolecules = IXA_FALSE;
    pBuilder->option_SaveOpt              = IXA_FALSE;
}


static void BUILDER_Update(IXA_STATUS_HANDLE hStatus,
                           INCHIBUILDER*      pBuilder)
{
    char            buffer[256];
#ifdef IXA_USES_NON_EX_CORE_API
    inchi_Input        input;
#else
    inchi_InputEx    input;
#endif
    inchi_Output    output;
    char            options[256];

    if (!pBuilder->dirty) return;

    options[0] = '\0';
    if (pBuilder->option_NEWPSOFF)
    {
        AppendOption(options, OPTION_PREFIX "NEWPSOFF");
    }
    if (pBuilder->option_DoNotAddH)
    {
        AppendOption(options, OPTION_PREFIX "DoNotAddH");
    }
    switch (pBuilder->option_stereo)
    {
        case IXA_INCHIBUILDER_STEREOOPTION_SAbs:
            break;
        case IXA_INCHIBUILDER_STEREOOPTION_SNon:
            AppendOption(options, OPTION_PREFIX "SNon");
            break;
        case IXA_INCHIBUILDER_STEREOOPTION_SRel:
            AppendOption(options, OPTION_PREFIX "SRel");
            break;
        case IXA_INCHIBUILDER_STEREOOPTION_SRac:
            AppendOption(options, OPTION_PREFIX "SRac");
            break;
        case IXA_INCHIBUILDER_STEREOOPTION_SUCF:
            AppendOption(options, OPTION_PREFIX "SUCF");
            break;
        default:
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR,
                "Stereo option %d is not recognised", pBuilder->option_stereo);
            return;
    }
    if (pBuilder->option_SUU)
    {
        AppendOption(options, OPTION_PREFIX "SUU");
    }
    if (pBuilder->option_SLUUD)
    {
        AppendOption(options, OPTION_PREFIX "SLUUD");
    }
    if (pBuilder->option_FixedH)
    {
        AppendOption(options, OPTION_PREFIX "FixedH");
    }
    if (pBuilder->option_RecMet)
    {
        AppendOption(options, OPTION_PREFIX "RecMet");
    }
    if (pBuilder->option_KET)
    {
        AppendOption(options, OPTION_PREFIX "KET");
    }
    if (pBuilder->option_15T)
    {
        AppendOption(options, OPTION_PREFIX "15T");
    }
    if (pBuilder->option_AuxNone)
    {
        AppendOption(options, OPTION_PREFIX "AuxNone");
    }
    if (pBuilder->option_Wnumber > 0)
    {
        sprintf(buffer, OPTION_PREFIX "W%d", pBuilder->option_Wnumber);
        AppendOption(options, buffer);
    }
    if (pBuilder->option_WarnOnEmptyStructure)
    {
        AppendOption(options, OPTION_PREFIX "WarnOnEmptyStructure");
    }
    if (pBuilder->option_LargeMolecules)
    {
        AppendOption(options, OPTION_PREFIX "LargeMolecules");
    }
    if (pBuilder->option_SaveOpt)
    {
        AppendOption(options, OPTION_PREFIX "SaveOpt");
    }
    if (pBuilder->molecule.chiral)
    {
        AppendOption(options, OPTION_PREFIX "ChiralFlagOn");
    }
    else
    {
        AppendOption(options, OPTION_PREFIX "ChiralFlagOff");
    }

    input.szOptions        =    options;
    input.num_atoms        =    pBuilder->molecule.num_atoms;
    input.atom            =    pBuilder->molecule.atom;
    input.num_stereo0D    =    pBuilder->molecule.num_stereo0D;
    input.stereo0D        =    pBuilder->molecule.stereo0D;

#ifdef IXA_USES_NON_EX_CORE_API
    switch (GetINCHI(&input, &output))
#else
    input.polymer        =    pBuilder->molecule.polymer;
    input.v3000            =    pBuilder->molecule.v3000;
    switch (GetINCHIEx(&input, &output))
#endif
    {
        case inchi_Ret_WARNING:
            STATUS_PushMessage(hStatus, IXA_STATUS_WARNING, output.szMessage);
            /* Deliberate fall-through. */

        case inchi_Ret_OKAY:
            inchi_free(pBuilder->inchi);
            pBuilder->inchi = NULL;
            if (output.szInChI)
            {
                pBuilder->inchi = (char *) inchi_malloc(strlen(output.szInChI) + 1);
                strcpy(pBuilder->inchi, output.szInChI);
            }

            inchi_free(pBuilder->auxinfo);
            pBuilder->auxinfo = NULL;
            if (output.szAuxInfo)
            {
                pBuilder->auxinfo = (char *) inchi_malloc(strlen(output.szAuxInfo) + 1);
                strcpy(pBuilder->auxinfo, output.szAuxInfo);
            }

            inchi_free(pBuilder->log);
            pBuilder->log = NULL;
            if (output.szLog)
            {
                pBuilder->log = (char *) inchi_malloc(strlen(output.szLog) + 1);
                strcpy(pBuilder->log, output.szLog);
            }
            break;

        default:
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, output.szMessage);
            break;
    }

    FreeINCHI(&output);

    pBuilder->dirty = IXA_FALSE;
}


IXA_INCHIBUILDER_HANDLE INCHI_DECL IXA_INCHIBUILDER_Create(IXA_STATUS_HANDLE hStatus)
{
    INCHIBUILDER* builder = (INCHIBUILDER*)inchi_malloc(sizeof(INCHIBUILDER));
    if (!builder)
    {
        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
        return NULL;
    }

    memset(&builder->molecule, 0, sizeof(builder->molecule));
    BUILDER_ClearOptions(builder);
    builder->dirty   = IXA_FALSE;
    builder->inchi   = NULL;
    builder->auxinfo = NULL;
    builder->log     = NULL;

    return BUILDER_Pack(builder);
}


void INCHI_DECL IXA_INCHIBUILDER_Destroy(IXA_STATUS_HANDLE  hStatus,
                                         IXA_INCHIBUILDER_HANDLE hBuilder)
{
    INCHIBUILDER* builder = BUILDER_Unpack(hStatus, hBuilder);
    if (!builder) return;

    ClearMolecule(&builder->molecule);
    inchi_free(builder->inchi);
    inchi_free(builder->auxinfo);
    inchi_free(builder->log);
    inchi_free(builder);
}


void INCHI_DECL IXA_INCHIBUILDER_SetMolecule(IXA_STATUS_HANDLE       hStatus,
                                             IXA_INCHIBUILDER_HANDLE hBuilder,
                                             IXA_MOL_HANDLE          hMolecule)
{
    int                 atom_index;
    IXA_ATOMID          atom;
    IXA_ATOMID          atom1;
    IXA_ATOMID          atom2;
    int                 bond_index;
    IXA_BONDID          bond;
    IXA_BOND_TYPE       bond_type;
    int                 stereo_index;
    IXA_STEREOID        stereo;
    IXA_STEREO_TOPOLOGY topology;
    int                 stereo_code;
    const char*         element;

    INCHIBUILDER* builder = BUILDER_Unpack(hStatus, hBuilder);
    if (!builder) return;

    ClearMolecule(&builder->molecule);

    builder->molecule.num_atoms = IXA_MOL_GetNumAtoms(hStatus, hMolecule);
    if (IXA_STATUS_HasError(hStatus)) return;

    if (builder->molecule.num_atoms > 0)
    {
        builder->molecule.atom = (inchi_Atom *) inchi_calloc(builder->molecule.num_atoms, sizeof(inchi_Atom));
        if (!builder->molecule.atom)
        {
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
            return;
        }

        for (atom_index = 0; atom_index < builder->molecule.num_atoms; atom_index++)
        {
            atom = IXA_MOL_GetAtomId(hStatus, hMolecule, atom_index);
            if (IXA_STATUS_HasError(hStatus)) return;
            element = IXA_MOL_GetAtomElement(hStatus, hMolecule, atom);
            if (IXA_STATUS_HasError(hStatus)) return;
            strcpy(builder->molecule.atom[atom_index].elname, element);
            builder->molecule.atom[atom_index].x = IXA_MOL_GetAtomX(hStatus, hMolecule, atom);
            if (IXA_STATUS_HasError(hStatus)) return;
            builder->molecule.atom[atom_index].y = IXA_MOL_GetAtomY(hStatus, hMolecule, atom);
            if (IXA_STATUS_HasError(hStatus)) return;
            builder->molecule.atom[atom_index].z = IXA_MOL_GetAtomZ(hStatus, hMolecule, atom);
            if (IXA_STATUS_HasError(hStatus)) return;
            builder->molecule.atom[atom_index].isotopic_mass = IXA_MOL_GetAtomMass(hStatus, hMolecule, atom);
            if (IXA_STATUS_HasError(hStatus)) return;
            builder->molecule.atom[atom_index].charge = IXA_MOL_GetAtomCharge(hStatus, hMolecule, atom);
            if (IXA_STATUS_HasError(hStatus)) return;
            builder->molecule.atom[atom_index].radical = IXA_MOL_GetAtomRadical(hStatus, hMolecule, atom);
            if (IXA_STATUS_HasError(hStatus)) return;
            builder->molecule.atom[atom_index].num_iso_H[0] = IXA_MOL_GetAtomHydrogens(hStatus, hMolecule, atom, 0);
            if (IXA_STATUS_HasError(hStatus)) return;
            builder->molecule.atom[atom_index].num_iso_H[1] = IXA_MOL_GetAtomHydrogens(hStatus, hMolecule, atom, 1);
            if (IXA_STATUS_HasError(hStatus)) return;
            builder->molecule.atom[atom_index].num_iso_H[2] = IXA_MOL_GetAtomHydrogens(hStatus, hMolecule, atom, 2);
            if (IXA_STATUS_HasError(hStatus)) return;
            builder->molecule.atom[atom_index].num_iso_H[3] = IXA_MOL_GetAtomHydrogens(hStatus, hMolecule, atom, 3);
            if (IXA_STATUS_HasError(hStatus)) return;
            builder->molecule.atom[atom_index].num_bonds = IXA_MOL_GetAtomNumBonds(hStatus, hMolecule, atom);
            if (IXA_STATUS_HasError(hStatus)) return;
            for (bond_index = 0; bond_index < builder->molecule.atom[atom_index].num_bonds; bond_index++)
            {
                bond = IXA_MOL_GetAtomBond(hStatus, hMolecule, atom, bond_index);
                if (IXA_STATUS_HasError(hStatus)) return;
                bond_type = IXA_MOL_GetBondType(hStatus, hMolecule, bond);
                if (IXA_STATUS_HasError(hStatus)) return;
                atom1 = IXA_MOL_GetBondAtom1(hStatus, hMolecule, bond);
                if (IXA_STATUS_HasError(hStatus)) return;
                atom2 = IXA_MOL_GetBondAtom2(hStatus, hMolecule, bond);
                if (IXA_STATUS_HasError(hStatus)) return;

                if (atom == atom1)
                {
                    builder->molecule.atom[atom_index].neighbor[bond_index] = IXA_MOL_GetAtomIndex(hStatus, hMolecule, atom2);
                    if (IXA_STATUS_HasError(hStatus)) return;
                }
                else
                {
                    if (atom == atom2)
                    {
                        builder->molecule.atom[atom_index].neighbor[bond_index] = IXA_MOL_GetAtomIndex(hStatus, hMolecule, atom1);
                        if (IXA_STATUS_HasError(hStatus)) return;
                    }
                    else
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Illegal bond detected");
                        return;
                    }
                }

                switch (bond_type)
                {
                    case IXA_BOND_TYPE_SINGLE:
                    {
                        IXA_BOND_WEDGE direction1;
                        IXA_BOND_WEDGE direction2;
                        direction1 = IXA_MOL_GetBondWedge(hStatus, hMolecule, bond, atom1);
                        if (IXA_STATUS_HasError(hStatus)) return;
                        direction2 = IXA_MOL_GetBondWedge(hStatus, hMolecule, bond, atom2);
                        if (IXA_STATUS_HasError(hStatus)) return;

                        if (atom == atom1)
                        {
                            stereo_code = GetSingleStereoCode(hStatus, direction1, direction2);
                            if (IXA_STATUS_HasError(hStatus)) return;
                        }
                        else
                        {
                            if (atom == atom2)
                            {
                                stereo_code = GetSingleStereoCode(hStatus, direction2, direction1);
                                if (IXA_STATUS_HasError(hStatus)) return;
                            }
                            else
                            {
                                STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Illegal bond detected");
                                return;
                            }
                        }
                    } break;

                    case IXA_BOND_TYPE_DOUBLE:
                    {
                        IXA_DBLBOND_CONFIG config;
                        config = IXA_MOL_GetDblBondConfig(hStatus, hMolecule, bond);
                        if (IXA_STATUS_HasError(hStatus)) return;
                        stereo_code = GetDoubleStereoCode(hStatus, config);
                        if (IXA_STATUS_HasError(hStatus)) return;
                    } break;

                    default:
                        stereo_code = INCHI_BOND_STEREO_NONE;
                        break;
                }

                builder->molecule.atom[atom_index].bond_stereo[bond_index] = stereo_code;
                builder->molecule.atom[atom_index].bond_type[bond_index]   = bond_type;
            }
        }
    }

    builder->molecule.num_stereo0D = IXA_MOL_GetNumStereos(hStatus, hMolecule);
    if (IXA_STATUS_HasError(hStatus)) return;
    if (builder->molecule.num_stereo0D > 0)
    {
        builder->molecule.stereo0D = (inchi_Stereo0D *) inchi_calloc(builder->molecule.num_stereo0D, sizeof(inchi_Stereo0D));
        if (!builder->molecule.stereo0D)
        {
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
            return;
        }

        for (stereo_index = 0; stereo_index < builder->molecule.num_stereo0D; stereo_index++)
        {
            stereo = IXA_MOL_GetStereoId(hStatus, hMolecule, stereo_index);
            if (IXA_STATUS_HasError(hStatus)) return;

            topology = IXA_MOL_GetStereoTopology(hStatus, hMolecule, stereo);
            if (IXA_STATUS_HasError(hStatus)) return;
            switch (topology)
            {
                case IXA_STEREO_TOPOLOGY_TETRAHEDRON:
                    builder->molecule.stereo0D[stereo_index].type = INCHI_StereoType_Tetrahedral;
                    atom = IXA_MOL_GetStereoCentralAtom(hStatus, hMolecule, stereo);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    builder->molecule.stereo0D[stereo_index].central_atom = IXA_MOL_GetAtomIndex(hStatus, hMolecule, atom);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    TranslateTetrahedralVertex(hStatus, hMolecule, stereo, &builder->molecule.stereo0D[stereo_index], 0);
                    TranslateTetrahedralVertex(hStatus, hMolecule, stereo, &builder->molecule.stereo0D[stereo_index], 1);
                    TranslateTetrahedralVertex(hStatus, hMolecule, stereo, &builder->molecule.stereo0D[stereo_index], 2);
                    TranslateTetrahedralVertex(hStatus, hMolecule, stereo, &builder->molecule.stereo0D[stereo_index], 3);
                    builder->molecule.stereo0D[stereo_index].parity = IXA_MOL_GetStereoParity(hStatus, hMolecule, stereo);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    break;

                case IXA_STEREO_TOPOLOGY_RECTANGLE:
                {
                    IXA_BONDID central_bond;
                    IXA_ATOMID internal1;
                    IXA_ATOMID vertex1a;
                    IXA_ATOMID vertex1b;
                    IXA_ATOMID internal2;
                    IXA_ATOMID vertex2a;
                    IXA_ATOMID vertex2b;
                    IXA_BOOL   flag;

                    central_bond = IXA_MOL_GetStereoCentralBond(hStatus, hMolecule, stereo);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    internal1 = IXA_MOL_GetBondAtom1(hStatus, hMolecule, central_bond);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    internal2 = IXA_MOL_GetBondAtom2(hStatus, hMolecule, central_bond);
                    if (IXA_STATUS_HasError(hStatus)) return;

                    vertex1a = IXA_MOL_GetStereoVertex(hStatus, hMolecule, stereo, 0);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    vertex1b = IXA_MOL_GetStereoVertex(hStatus, hMolecule, stereo, 1);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    vertex2a = IXA_MOL_GetStereoVertex(hStatus, hMolecule, stereo, 2);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    vertex2b = IXA_MOL_GetStereoVertex(hStatus, hMolecule, stereo, 3);
                    if (IXA_STATUS_HasError(hStatus)) return;

                    flag = IsRectOrAntiRectCentre(hStatus, hMolecule,
                        vertex1a, vertex1b, internal1, vertex2a, vertex2b, internal2);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    if (!flag)
                    {
                        /* Internal and external vertices as currently assigned
                            do not constitute a valid olefin. Try
                            swapping the internal vertices. */
                        atom      = internal1;
                        internal1 = internal2;
                        internal2 = atom;

                        flag = IsRectOrAntiRectCentre(hStatus, hMolecule,
                            vertex1a, vertex1b, internal1, vertex2a, vertex2b, internal2);
                        if (IXA_STATUS_HasError(hStatus)) return;
                        if (!flag)
                        {
                            /* The stereo centre cannot be an olefin.
                                Perhaps it is a cumulene? */
                            internal1 = ExtendCumulene(hStatus, hMolecule, central_bond, internal1);
                            if (IXA_STATUS_HasError(hStatus)) return;
                            internal2 = ExtendCumulene(hStatus, hMolecule, central_bond, internal2);
                            if (IXA_STATUS_HasError(hStatus)) return;

                            flag = IsRectOrAntiRectCentre(hStatus, hMolecule,
                                vertex1a, vertex1b, internal1, vertex2a, vertex2b, internal2);
                            if (IXA_STATUS_HasError(hStatus)) return;
                            if (!flag)
                            {
                                /* Internal and external vertices as currently assigned
                                    do not constitute a valid cumulene. Try
                                   swapping the internal vertices. */
                                atom      = internal1;
                                internal1 = internal2;
                                internal2 = atom;

                                flag = IsRectOrAntiRectCentre(hStatus, hMolecule,
                                    vertex1a, vertex1b, internal1, vertex2a, vertex2b, internal2);
                                if (IXA_STATUS_HasError(hStatus)) return;
                                if (!flag)
                                {
                                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Stereo centre cannot be either an olefin or a cumulenee");
                                    return;
                                }
                            }
                        }
                    }
                    builder->molecule.stereo0D[stereo_index].type         = INCHI_StereoType_DoubleBond;
                    builder->molecule.stereo0D[stereo_index].central_atom = NO_ATOM;
                    builder->molecule.stereo0D[stereo_index].neighbor[1]  = IXA_MOL_GetAtomIndex(hStatus, hMolecule, internal1);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    builder->molecule.stereo0D[stereo_index].neighbor[2]  = IXA_MOL_GetAtomIndex(hStatus, hMolecule, internal2);
                    if (IXA_STATUS_HasError(hStatus)) return;

                    if ((vertex1a != IXA_ATOMID_IMPLICIT_H) && (vertex2b != IXA_ATOMID_IMPLICIT_H))
                    {
                        builder->molecule.stereo0D[stereo_index].neighbor[0] = IXA_MOL_GetAtomIndex(hStatus, hMolecule, vertex1a);
                        if (IXA_STATUS_HasError(hStatus)) return;
                        builder->molecule.stereo0D[stereo_index].neighbor[3] = IXA_MOL_GetAtomIndex(hStatus, hMolecule, vertex2b);
                        if (IXA_STATUS_HasError(hStatus)) return;
                    }
                    else if ((vertex1b != IXA_ATOMID_IMPLICIT_H) && (vertex2a != IXA_ATOMID_IMPLICIT_H))
                    {
                        builder->molecule.stereo0D[stereo_index].neighbor[0] = IXA_MOL_GetAtomIndex(hStatus, hMolecule, vertex1b);
                        if (IXA_STATUS_HasError(hStatus)) return;
                        builder->molecule.stereo0D[stereo_index].neighbor[3] = IXA_MOL_GetAtomIndex(hStatus, hMolecule, vertex2a);
                        if (IXA_STATUS_HasError(hStatus)) return;
                    }
                    else
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Cannot create internal representation of stereo centre");
                        return;
                    }

                    builder->molecule.stereo0D[stereo_index].parity = IXA_MOL_GetStereoParity(hStatus, hMolecule, stereo);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    break;
                }

                case IXA_STEREO_TOPOLOGY_ANTIRECTANGLE:
                {
                    IXA_ATOMID central_atom;
                    IXA_ATOMID internal1;
                    IXA_ATOMID vertex1a;
                    IXA_ATOMID vertex1b;
                    IXA_ATOMID internal2;
                    IXA_ATOMID vertex2a;
                    IXA_ATOMID vertex2b;
                    IXA_BOOL   flag;

                    central_atom = IXA_MOL_GetStereoCentralAtom(hStatus, hMolecule, stereo);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    ExtendAllene(hStatus, hMolecule, central_atom, &internal1, &internal2);
                    if (IXA_STATUS_HasError(hStatus)) return;

                    vertex1a = IXA_MOL_GetStereoVertex(hStatus, hMolecule, stereo, 0);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    vertex1b = IXA_MOL_GetStereoVertex(hStatus, hMolecule, stereo, 1);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    vertex2a = IXA_MOL_GetStereoVertex(hStatus, hMolecule, stereo, 2);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    vertex2b = IXA_MOL_GetStereoVertex(hStatus, hMolecule, stereo, 3);
                    if (IXA_STATUS_HasError(hStatus)) return;

                    flag = IsRectOrAntiRectCentre(hStatus, hMolecule,
                        vertex1a, vertex1b, internal1, vertex2a, vertex2b, internal2);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    if (!flag)
                    {
                        /* Internal and external vertices as currently assigned
                            do not constitute a valid cumulene. Try
                            swapping the internal vertices. */
                        atom      = internal1;
                        internal1 = internal2;
                        internal2 = atom;

                        flag = IsRectOrAntiRectCentre(hStatus, hMolecule,
                            vertex1a, vertex1b, internal1, vertex2a, vertex2b, internal2);
                        if (IXA_STATUS_HasError(hStatus)) return;
                        if (!flag)
                        {
                            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Stereo centre cannot be either an olefin or a cumulenee");
                            return;
                        }
                    }

                    builder->molecule.stereo0D[stereo_index].type         = INCHI_StereoType_Allene;
                    builder->molecule.stereo0D[stereo_index].central_atom = IXA_MOL_GetAtomIndex(hStatus, hMolecule, central_atom);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    builder->molecule.stereo0D[stereo_index].neighbor[1]  = IXA_MOL_GetAtomIndex(hStatus, hMolecule, internal1);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    builder->molecule.stereo0D[stereo_index].neighbor[2]  = IXA_MOL_GetAtomIndex(hStatus, hMolecule, internal2);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    if ((vertex1a != IXA_ATOMID_IMPLICIT_H) && (vertex2b != IXA_ATOMID_IMPLICIT_H))
                    {
                        builder->molecule.stereo0D[stereo_index].neighbor[0] = IXA_MOL_GetAtomIndex(hStatus, hMolecule, vertex1a);
                        builder->molecule.stereo0D[stereo_index].neighbor[3] = IXA_MOL_GetAtomIndex(hStatus, hMolecule, vertex2b);
                    }
                    else if ((vertex1b != IXA_ATOMID_IMPLICIT_H) && (vertex2a != IXA_ATOMID_IMPLICIT_H))
                    {
                        builder->molecule.stereo0D[stereo_index].neighbor[0] = IXA_MOL_GetAtomIndex(hStatus, hMolecule, vertex1b);
                        builder->molecule.stereo0D[stereo_index].neighbor[3] = IXA_MOL_GetAtomIndex(hStatus, hMolecule, vertex2a);
                    }
                    else
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Cannot create internal representation of stereo centre");
                        return;
                    }
                    builder->molecule.stereo0D[stereo_index].parity = IXA_MOL_GetStereoParity(hStatus, hMolecule, stereo);
                    if (IXA_STATUS_HasError(hStatus)) return;
                    break;
                }

                default:
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Unexpected stereo topology");
                    return;
            }
        }
    }

    builder->molecule.chiral = IXA_MOL_GetChiral(hStatus, hMolecule);

    /* Extended input features (TODO: replace and isolate)    */
    {
        int k, m;
        INCHIMOL* molecule = MOL_Unpack(hStatus, hMolecule);
        if ( molecule )
        {
            int nat = IXA_MOL_GetNumAtoms( hStatus, hMolecule);

            /* Polymer */
            if ( molecule->polymer )
            {
                builder->molecule.polymer = (inchi_Input_Polymer *) inchi_calloc( 1, sizeof(inchi_Input_Polymer) );
                if ( !builder->molecule.polymer )
                {
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
                    return;
                }
                builder->molecule.polymer->n    = molecule->polymer->n;
                if ( builder->molecule.polymer->n )
                {
                    builder->molecule.polymer->units = (inchi_Input_PolymerUnit **) inchi_calloc( builder->molecule.polymer->n, sizeof(builder->molecule.polymer->units[0]) );
                    if ( !builder->molecule.polymer->units )
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
                        return;
                    }
                    memset( builder->molecule.polymer->units, 0, sizeof( *(builder->molecule.polymer->units) ) );
                }
                for (k=0; k<builder->molecule.polymer->n; k++ )
                {
                    int q=0;
                    INCHIMOL_SGROUP    *     groupk    =    molecule->polymer->units[k];
                    inchi_Input_PolymerUnit    *     unitk    =    builder->molecule.polymer->units[k] = (inchi_Input_PolymerUnit *) inchi_calloc( 1, sizeof(inchi_Input_PolymerUnit) );
                    if (!unitk )
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
                        return;
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
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
                        return;
                    }
                    for (m=0; m<unitk->na; m++)
                    {
                        unitk->alist[m] = groupk->alist[m];
                    }
                    unitk->nb = groupk->nb;
                    if ( unitk->nb > 0 )
                    {
                        unitk->blist = (int *) inchi_calloc( 2*unitk->nb, sizeof(int) );
                        if (!unitk->blist )
                        {
                            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
                            return;
                        }
                        for (m=0; m < 2*unitk->nb; m++)
                        {
                            unitk->blist[m]    = groupk->blist[m];
                        }
                    }
                    else
                        unitk->blist = NULL;
                }
            }

            /* V3000 */
            if ( molecule->v3000 )
            {
                int m, k, nn;

                builder->molecule.v3000    = (inchi_Input_V3000 *) inchi_calloc( 1, sizeof(inchi_Input_V3000) );
                if ( !builder->molecule.v3000 )
                {
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
                    return;
                }
                memset( builder->molecule.v3000, 0, sizeof( inchi_Input_V3000 ) );

                builder->molecule.v3000->n_collections        = molecule->v3000->n_collections;
                builder->molecule.v3000->n_haptic_bonds        = molecule->v3000->n_haptic_bonds;
                builder->molecule.v3000->n_non_haptic_bonds    = molecule->v3000->n_non_haptic_bonds;
                builder->molecule.v3000->n_sgroups            = molecule->v3000->n_sgroups;
                builder->molecule.v3000->n_non_star_atoms    = molecule->v3000->n_non_star_atoms;
                builder->molecule.v3000->n_star_atoms        = molecule->v3000->n_star_atoms;
                builder->molecule.v3000->n_steabs            = molecule->v3000->n_steabs;
                builder->molecule.v3000->n_sterac            = molecule->v3000->n_sterac;
                builder->molecule.v3000->n_sterel            = molecule->v3000->n_sterel;
                builder->molecule.v3000->n_3d_constraints    = molecule->v3000->n_3d_constraints;

                if ( molecule->v3000->atom_index_orig )
                {
                    builder->molecule.v3000->atom_index_orig = (int *) inchi_calloc( nat, sizeof(int) );
                    if ( NULL==builder->molecule.v3000->atom_index_orig )
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
                        return;
                    }
                    memcpy( builder->molecule.v3000->atom_index_orig, molecule->v3000->atom_index_orig, nat );
                }
                if ( molecule->v3000->atom_index_fin )
                {
                    builder->molecule.v3000->atom_index_fin = (int *) inchi_calloc( nat, sizeof(int) );
                    if ( NULL==builder->molecule.v3000->atom_index_fin )
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
                        return;
                    }
                    memcpy( builder->molecule.v3000->atom_index_fin, molecule->v3000->atom_index_fin, nat);
                }
                if ( molecule->v3000->n_haptic_bonds && molecule->v3000->lists_haptic_bonds )
                {
                    builder->molecule.v3000->lists_haptic_bonds = (int **) calloc( molecule->v3000->n_haptic_bonds, sizeof (int*) );
                    if ( NULL==builder->molecule.v3000->lists_haptic_bonds )
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
                        return;
                    }
                    for (m=0; m<molecule->v3000->n_haptic_bonds; m++)
                    {
                        int *lst=NULL;
                        int *mol_lst = molecule->v3000->lists_haptic_bonds[m];
                        int n_endpts = mol_lst[2];
                        nn = n_endpts + 3;
                        lst = builder->molecule.v3000->lists_haptic_bonds[m] = (int *) calloc( nn, sizeof (int) );
                        if ( NULL==lst )
                        {
                            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
                            return;
                        }
                        for (k=0; k<nn; k++)
                            lst[k] = mol_lst[k];
                    }
                }
                if ( molecule->v3000->n_steabs && molecule->v3000->lists_steabs )
                {
                    builder->molecule.v3000->lists_steabs = (int **) calloc( molecule->v3000->n_steabs, sizeof (int*) );
                    if ( NULL==builder->molecule.v3000->lists_steabs )
                    {
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
                        return;
                    }
                    for (m=0; m<molecule->v3000->n_steabs; m++)
                    {
                        int *lst=NULL;
                        int *mol_lst = molecule->v3000->lists_steabs[m];
                        nn = mol_lst[1] + 2;
                        lst = builder->molecule.v3000->lists_steabs[m] = (int *) calloc( nn, sizeof (int) );
                        if ( NULL==lst )
                        {
                            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Out of memory");
                            return;
                        }
                        for (k=0; k<nn; k++)
                            lst[k] = mol_lst[k];
                    }
                }
            }
        }
    }

    if (IXA_STATUS_HasError(hStatus)) return;

    builder->dirty = IXA_TRUE;
}


void INCHI_DECL IXA_INCHIBUILDER_SetOption_Stereo(IXA_STATUS_HANDLE             hStatus,
                                                  IXA_INCHIBUILDER_HANDLE       hBuilder,
                                                  IXA_INCHIBUILDER_STEREOOPTION vValue)
{
    INCHIBUILDER* builder = BUILDER_Unpack(hStatus, hBuilder);
    if (!builder) return;

    builder->option_stereo = vValue;
}


void INCHI_DECL IXA_INCHIBUILDER_SetOption_Timeout(IXA_STATUS_HANDLE       hStatus,
                                                   IXA_INCHIBUILDER_HANDLE hBuilder,
                                                   int                     vValue)
{
    INCHIBUILDER* builder = BUILDER_Unpack(hStatus, hBuilder);
    if (!builder) return;

    builder->option_Wnumber = vValue;
}


void INCHI_DECL IXA_INCHIBUILDER_SetOption(IXA_STATUS_HANDLE       hStatus,
                                           IXA_INCHIBUILDER_HANDLE hBuilder,
                                           IXA_INCHIBUILDER_OPTION vOption,
                                           IXA_BOOL                vValue)
{
    INCHIBUILDER* builder = BUILDER_Unpack(hStatus, hBuilder);
    if (!builder) return;

    switch (vOption)
    {
         case IXA_INCHIBUILDER_OPTION_NewPsOff:
             builder->option_NEWPSOFF = vValue;
             return;
         case IXA_INCHIBUILDER_OPTION_DoNotAddH:
             builder->option_DoNotAddH = vValue;
             return;
         case IXA_INCHIBUILDER_OPTION_SUU:
             builder->option_SUU = vValue;
             return;
         case IXA_INCHIBUILDER_OPTION_SLUUD:
             builder->option_SLUUD = vValue;
             return;
         case IXA_INCHIBUILDER_OPTION_FixedH:
             builder->option_FixedH = vValue;
             return;
         case IXA_INCHIBUILDER_OPTION_RecMet:
             builder->option_RecMet = vValue;
             return;
         case IXA_INCHIBUILDER_OPTION_KET:
             builder->option_KET = vValue;
             return;
         case IXA_INCHIBUILDER_OPTION_15T:
             builder->option_15T = vValue;
             return;
         case IXA_INCHIBUILDER_OPTION_SaveOpt:
             builder->option_SaveOpt = vValue;
             return;
         case IXA_INCHIBUILDER_OPTION_AuxNone:
             builder->option_AuxNone = vValue;
             return;
         case IXA_INCHIBUILDER_OPTION_WarnOnEmptyStructure:
             builder->option_WarnOnEmptyStructure = vValue;
             return;
         case IXA_INCHIBUILDER_OPTION_LargeMolecules:
             builder->option_LargeMolecules = vValue;
             return;
        default:
             STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Option is not recognised");
             return;
    }
}


const char* INCHI_DECL IXA_INCHIBUILDER_GetInChI(IXA_STATUS_HANDLE       hStatus,
                                                 IXA_INCHIBUILDER_HANDLE hBuilder)
{
    INCHIBUILDER* builder = BUILDER_Unpack(hStatus, hBuilder);
    if (!builder) return NULL;

    BUILDER_Update(hStatus, builder);
    return builder->inchi;
}


const char* INCHI_DECL IXA_INCHIBUILDER_GetInChIEx(IXA_STATUS_HANDLE       hStatus,
                                                 IXA_INCHIBUILDER_HANDLE hBuilder)
{
    INCHIBUILDER* builder = BUILDER_Unpack(hStatus, hBuilder);
    if (!builder) return NULL;

    BUILDER_Update(hStatus, builder);
    return builder->inchi;
}


const char* INCHI_DECL IXA_INCHIBUILDER_GetAuxInfo(IXA_STATUS_HANDLE       hStatus,
                                                   IXA_INCHIBUILDER_HANDLE hBuilder)
{
    INCHIBUILDER* builder = BUILDER_Unpack(hStatus, hBuilder);
    if (!builder) return NULL;

    BUILDER_Update(hStatus, builder);
    return builder->auxinfo;
}


const char* INCHI_DECL IXA_INCHIBUILDER_GetLog(IXA_STATUS_HANDLE       hStatus,
                                               IXA_INCHIBUILDER_HANDLE hBuilder)
{
    INCHIBUILDER* builder = BUILDER_Unpack(hStatus, hBuilder);
    if (!builder) return NULL;

    BUILDER_Update(hStatus, builder);
    return builder->log;
}
