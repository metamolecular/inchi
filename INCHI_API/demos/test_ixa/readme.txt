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


This directory contains example of using newly added in v. 1.05 
Extensible InChI API (IXA).

The demo program test_ixa reads input Molfile/SDFile and 
generates InChI; it also demonstrates an organization  of
verification round-trip "Structure->InChI->Structure->InChI"
via InChI API calls.

The source codes are placed in sub-directory 'src';
gcc/Linux makefiles in sub-directory 'gcc';
MS VS 2008 project is placed in sub-directory 'vc9'.

The created binaries are saved in upper-level directory 'bin'.
