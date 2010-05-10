/*
 * caffeine.h
 *
 * WARNING, WARNING!  LICENSE JUNK!
 *
 * Licensed under the UIUC/NCSA open source license.
 *
 * Copyright (c) 2010 ACM SIGEmbedded
 * All rights reserved.
 *
 * Developed by:     ACM SIGEmbedded
 *                   University of Illinois ACM student chapter
 *                   http://www.acm.illinois.edu/sigembedded/caffeine
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal with the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 *   - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimers.
 *
 *   - Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following disclaimers
 *   in the documentation and/or other materials provided with the
 *   distribution.
 *
 *   - Neither the names of ACM SIGEmbedded, University of Illinois, UIUC
 *   ACM student chapter, nor the names of its contributors may be used to endorse
 *   or promote products derived from this Software without specific prior
 *   written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
 *
 *
 * Now that that's all done...
 *
 * Function prototypes for caffeine.c go here.
 *
 */



#ifndef CAFFEINE_H
#define CAFFEINE_H

void send_ack();
void vend(char selection);
void send_card_data();
char check_card_data();
void reset_card_data();
void send_error();
void send_string(char *string);

#endif
