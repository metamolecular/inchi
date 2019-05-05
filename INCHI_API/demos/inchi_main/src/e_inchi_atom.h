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


#ifndef __INCHI_ATOM_H__
#define __INCHI_ATOM_H__

#ifndef COMPILE_ALL_CPP
#ifdef __cplusplus
extern "C" {
#endif
#endif

void           e_FreeInchi_Atom( inchi_Atom **at );
void           e_FreeInchi_Stereo0D( inchi_Stereo0D **stereo0D );
inchi_Atom     *e_CreateInchi_Atom( int num_atoms );
inchi_Stereo0D *e_CreateInchi_Stereo0D( int num_stereo0D );
void           e_FreeInchi_Input( inchi_Input *inp_at_data );
int            e_RemoveRedundantNeighbors( inchi_Input *inp_at_data );

#ifndef COMPILE_ALL_CPP
#ifdef __cplusplus
}
#endif
#endif


#endif /* __INCHI_ATOM_H__ */
