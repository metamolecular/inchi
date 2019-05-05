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


#ifndef __READSTRU_H__
#define __READSTRU_H__


#ifndef COMPILE_ALL_CPP
#ifdef __cplusplus
extern "C" {
#endif
#endif



int e_ReadStructure(STRUCT_DATA *sd, INPUT_PARMS *ip,
                    INCHI_IOSTREAM *inp_file,
                    INCHI_IOSTREAM *log_file, INCHI_IOSTREAM *output_file, INCHI_IOSTREAM *prb_file,
                    inchi_Input *pInp, long num_inp,  /* for CML:*/ int inp_index, int *out_index );


#ifndef COMPILE_ALL_CPP
#ifdef __cplusplus
}
#endif
#endif


#endif /* __READSTRU_H__ */
