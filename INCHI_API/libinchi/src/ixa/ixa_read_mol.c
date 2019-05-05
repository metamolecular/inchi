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


#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../../../../INCHI_BASE/src/mode.h"
#include "../../../../INCHI_BASE/src/inchi_api.h"
#include "../../../../INCHI_BASE/src/ichicomp.h"
#include "../../../../INCHI_BASE/src/util.h"
#include "../../../../INCHI_BASE/src/mol_fmt.h"
#include "../../../../INCHI_BASE/src/ichi_io.h"

#include "ixa_status.h"


/* #define memicmp strncasecmp */

int IXA_MOL_SetExtMoldataByMolfileExtInput( IXA_STATUS_HANDLE hStatus,
                                        IXA_MOL_HANDLE    hMolecule,
                                        MOL_FMT_DATA* mfdata);



/*
    Read Molfile (throuh call to InChI native ReadMolfile() ) and pack the data into INCHIMOL
*/
void INCHI_DECL IXA_MOL_ReadMolfile( IXA_STATUS_HANDLE hStatus,
                                     IXA_MOL_HANDLE    hMolecule,
                                     const char*       pBytes)
{
    int err;
    char error_string[STR_ERR_LEN];
    MOL_FMT_DATA*    mol_data = NULL;
    MOL_FMT_CTAB     OnlyCtab;
    int i, n1;
    IXA_ATOMID    atom1;
    IXA_ATOMID    atom2;
    IXA_BONDID    bond;
    int           bond_count;
    IXA_BOND_TYPE bond_type;
    const char*   p_cursor;
    INCHI_IOSTREAM instr;
    INCHI_IOSTREAM *inp_file = &instr;
    S_CHAR orig_mass_diff;
    int mass_or_mass_diff;
    int inlen=0;


    /* Throw out any existing content in the IXA molecule object. */
    IXA_MOL_Clear(hStatus, hMolecule);

    /* Wrap input text into an INCHI_IOSTREAM_STRING to use    a     */
    /* general-purpose (Molfile->InChI internal data) reader    */
    inchi_ios_init(inp_file, INCHI_IOSTREAM_TYPE_STRING, NULL);
    inp_file->s.pStr = (char * ) pBytes;
    inlen = strlen( inp_file->s.pStr ) + 1;
    inp_file->s.nUsedLength = inlen;
    inp_file->s.nPtr = 0;
    inp_file->f = NULL;

    error_string[0] = '\0';

    /* Parse the MolFile and store its content in an internal data structure. It might be possible
       to eliminate the internal struct and read directly into the IXA molecule object but that
       would mean rewriting a lot of code and also mask the relationship between this MolFile
       reading function and those used in previous InChI code. */
    p_cursor = pBytes;
    mol_data = ReadMolfile( inp_file,
                            NULL,
                            &OnlyCtab,
                            0, /*NULL != szCoord,*/
                            1,
                            NULL, 0, NULL, NULL, NULL,
                            &err, error_string );

    /* uses "err" and "error_string" to report errors. Pick up any errors
       and write them to the status object. */
    if (!mol_data)
    {
        if (error_string[0])
        {
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, error_string);
        }
        else
        {
            STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "No molfile data or failed to read");
        }
        goto exit_function;
    }
    else
    {
        if (error_string[0])
        {
            if ( !err)
                STATUS_PushMessage(hStatus, IXA_STATUS_WARNING, error_string);
        }
    }

    /* No need to do anything if the MolFile does not contain any atoms. */
    if (mol_data->ctab.n_atoms > 0)
    {
        /* Create the appropriate number of IXA atoms. Some atom properties can only be set
           once bonds have been created and bonds can only be created once all atoms are in
           place. Hence this rather lightweight loop that creates atoms but does not set
           their properties. */
        for (i = 0; i < mol_data->ctab.n_atoms; i++)
        {
            IXA_MOL_CreateAtom(hStatus, hMolecule);
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;
        }

        /* All atoms now exist so it is now possible to create bonds and set their properties. */
        for ( i = 0; i < mol_data->ctab.n_bonds; i ++ )
        {
            /* Create the bond. */
            atom1 = IXA_MOL_GetAtomId(hStatus, hMolecule, mol_data->ctab.bonds[i].atnum1-1);
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;
            atom2 = IXA_MOL_GetAtomId(hStatus, hMolecule, mol_data->ctab.bonds[i].atnum2-1);
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;
            bond = IXA_MOL_CreateBond(hStatus, hMolecule, atom1, atom2);
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;

            /* Set bond order. */
            switch (mol_data->ctab.bonds[i].bond_type)
            {
                case 1:
                    IXA_MOL_SetBondType(hStatus, hMolecule, bond, IXA_BOND_TYPE_SINGLE);
                    if (IXA_STATUS_HasError(hStatus)) goto exit_function;
                    break;
                case 2:
                    IXA_MOL_SetBondType(hStatus, hMolecule, bond, IXA_BOND_TYPE_DOUBLE);
                    if (IXA_STATUS_HasError(hStatus)) goto exit_function;
                    break;
                case 3:
                    IXA_MOL_SetBondType(hStatus, hMolecule, bond, IXA_BOND_TYPE_TRIPLE);
                    if (IXA_STATUS_HasError(hStatus)) goto exit_function;
                    break;
                case 4:
                    IXA_MOL_SetBondType(hStatus, hMolecule, bond, IXA_BOND_TYPE_AROMATIC);
                    if (IXA_STATUS_HasError(hStatus)) goto exit_function;
                    break;
                default:
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Bond type %d is not recognised", mol_data->ctab.bonds[i].bond_type);
                    goto exit_function;
            }

            /* Set bond wedge and double bond configuration. */
            switch (mol_data->ctab.bonds[i].bond_stereo)
            {
                case INPUT_STEREO_DBLE_EITHER:
                    IXA_MOL_SetDblBondConfig(hStatus, hMolecule, bond, IXA_DBLBOND_CONFIG_EITHER);
                    if (IXA_STATUS_HasError(hStatus)) goto exit_function;
                    break;
                case INPUT_STEREO_SNGL_UP:
                    IXA_MOL_SetBondWedge(hStatus, hMolecule, bond, atom1, IXA_BOND_WEDGE_UP);
                    if (IXA_STATUS_HasError(hStatus)) goto exit_function;
                    break;
                case INPUT_STEREO_SNGL_EITHER:
                    IXA_MOL_SetBondWedge(hStatus, hMolecule, bond, atom1, IXA_BOND_WEDGE_EITHER);
                    if (IXA_STATUS_HasError(hStatus)) goto exit_function;
                    break;
                case INPUT_STEREO_SNGL_DOWN:
                    IXA_MOL_SetBondWedge(hStatus, hMolecule, bond, atom1, IXA_BOND_WEDGE_DOWN);
                    if (IXA_STATUS_HasError(hStatus)) goto exit_function;
                    break;
                case 0:
                    IXA_MOL_SetBondWedge(hStatus, hMolecule, bond, atom1, IXA_BOND_WEDGE_NONE);
                    if (IXA_STATUS_HasError(hStatus)) goto exit_function;
                    break;
                default:
                    STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Stereo type %d is not recognised", mol_data->ctab.bonds[i].bond_stereo);
                    goto exit_function;
            }
        }

        /* Now that all bonds have been dealt with, set atom properties. */
        for (i = 0; i < mol_data->ctab.n_atoms; i++)
        {
            /* Most atom properties are easily dealt with. They might equally have been set
               when the atom was first created. */
            atom1 = IXA_MOL_GetAtomId(hStatus, hMolecule, i);
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;
            IXA_MOL_SetAtomElement(hStatus, hMolecule, atom1, mol_data->ctab.atoms[i].symbol);
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;

            /*    Fix 2016-02-18/03-04: conform GetINCHI() conventions, account for the most abundant isotope */
            orig_mass_diff = mol_data->ctab.atoms[i].mass_difference;
            if ( orig_mass_diff == 0 )
            {
                /* not isotopic */
                mass_or_mass_diff = 0;
            }
            else if ( orig_mass_diff == ZERO_ATW_DIFF )
            {
                /*    isotopic enrichment, special case:
                    isotope's integer mass is equal to the
                    rounded average mass from Periodic Table
                */
                mass_or_mass_diff = 0 + ISOTOPIC_SHIFT_FLAG;
            }
            else
            {
                /* isotopic mass, general case */
                mass_or_mass_diff = orig_mass_diff + ISOTOPIC_SHIFT_FLAG;
            }
            IXA_MOL_SetAtomMass( hStatus, hMolecule, atom1, mass_or_mass_diff );
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;
            IXA_MOL_SetAtomCharge(hStatus, hMolecule, atom1, mol_data->ctab.atoms[i].charge);
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;
            IXA_MOL_SetAtomX(hStatus, hMolecule, atom1, mol_data->ctab.atoms[i].fx);
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;
            IXA_MOL_SetAtomY(hStatus, hMolecule, atom1, mol_data->ctab.atoms[i].fy);
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;
            IXA_MOL_SetAtomZ(hStatus, hMolecule, atom1, mol_data->ctab.atoms[i].fz);
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;
#if( SINGLET_IS_TRIPLET == 1 )
            if (mol_data->ctab.atoms[i].radical == 1)
            {
                IXA_MOL_SetAtomRadical(hStatus, hMolecule, atom1, IXA_ATOM_RADICAL_TRIPLET);
                if (IXA_STATUS_HasError(hStatus)) goto exit_function;
            }
            else
#endif
            IXA_MOL_SetAtomRadical(hStatus, hMolecule, atom1, (IXA_ATOM_RADICAL) mol_data->ctab.atoms[i].radical);
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;

            /* Implicit hydrogen count depends on an analysis of the bonds connected to the
               atom. This is the reason why bonds had to be created before atom properties
               could be set. */
            bond_count = IXA_MOL_GetAtomNumBonds(hStatus, hMolecule, atom1);
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;
            if (mol_data->ctab.atoms[i].valence &&
                (mol_data->ctab.atoms[i].valence != 15 || (bond_count != 0)))
            {
                /* Molfile contains special valence => calculate number of H */
                int valence;
                int chem_bonds_valence = 0;
                int aromatic_count = 0;
                valence = mol_data->ctab.atoms[i].valence; /*  save atom valence if available */
                for ( n1 = 0; n1 < bond_count; n1 ++ )
                {
                    bond = IXA_MOL_GetAtomBond(hStatus, hMolecule, atom1, n1);
                    if (IXA_STATUS_HasError(hStatus)) goto exit_function;
                    bond_type = IXA_MOL_GetBondType(hStatus, hMolecule, bond);
                    if (IXA_STATUS_HasError(hStatus)) goto exit_function;
                    switch (bond_type)
                    {
                        case INCHI_BOND_TYPE_SINGLE:
                            chem_bonds_valence += 1;
                            break;
                        case INCHI_BOND_TYPE_DOUBLE:
                            chem_bonds_valence += 2;
                            break;
                        case INCHI_BOND_TYPE_TRIPLE:
                            chem_bonds_valence += 3;
                            break;
                        case INCHI_BOND_TYPE_ALTERN:
                            chem_bonds_valence += 1;
                            aromatic_count++;
                            break;
                    }
                }
                switch (aromatic_count)
                {
                    case 0:
                        break;
                    case 2:
                    case 3:
                        chem_bonds_valence++;
                        break;
                    default:
                        STATUS_PushMessage(hStatus, IXA_STATUS_ERROR, "Atom cannot have %d aromatic bonds", aromatic_count);
                        goto exit_function;
                }
                if ( valence >= chem_bonds_valence )
                {
                    IXA_MOL_SetAtomHydrogens(hStatus, hMolecule, atom1, 0, valence - chem_bonds_valence);
                    if (IXA_STATUS_HasError(hStatus)) goto exit_function;
                }
            }
            else if ( mol_data->ctab.atoms[i].atom_aliased_flag )
            {
                IXA_MOL_SetAtomHydrogens(hStatus, hMolecule, atom1, 0, 0);
                if (IXA_STATUS_HasError(hStatus)) goto exit_function;
            }
            else if ( mol_data->ctab.atoms[i].valence == 15 && (bond_count == 0))
            {
                IXA_MOL_SetAtomHydrogens(hStatus, hMolecule, atom1, 0, 0);
                if (IXA_STATUS_HasError(hStatus)) goto exit_function;
            }
            else
            {
                IXA_MOL_SetAtomHydrogens(hStatus, hMolecule, atom1, 0, -1);
            }
        }

        /* Chiral flag is a whole-molecule property not tied to any particular atom, bond
           or stereo centre. */
        if (mol_data->ctab.chiral_flag)
        {
            IXA_MOL_SetChiral(hStatus, hMolecule, IXA_TRUE);
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;
        }

        if ( mol_data->ctab.sgroups.used || mol_data->ctab.v3000 )
        {
            /* Treat Molfile extended data (polymers; V300 features ) */
            IXA_MOL_SetExtMoldataByMolfileExtInput( hStatus, hMolecule, mol_data );
            if (IXA_STATUS_HasError(hStatus)) goto exit_function;
         }
    }

exit_function:
    FreeMolfileData( mol_data );
}
