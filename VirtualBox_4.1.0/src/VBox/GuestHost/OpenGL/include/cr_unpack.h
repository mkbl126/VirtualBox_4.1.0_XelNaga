/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_UNPACK_H
#define CR_UNPACK_H

#include "cr_compiler.h"
#include "cr_spu.h"
#include "cr_protocol.h"
#include "cr_mem.h"

#include <iprt/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern SPUDispatchTable cr_unpackDispatch;
/*extern DLLDATA(const unsigned char *) cr_unpackData;*/
extern DECLEXPORT(const unsigned char *) cr_unpackData;

DECLEXPORT(void) crUnpackSetReturnPointer( CRNetworkPointer *ptr );
DECLEXPORT(void) crUnpackSetWritebackPointer( CRNetworkPointer *ptr );
DECLEXPORT(void) crUnpack( const void *data, const void *opcodes, unsigned int num_opcodes, SPUDispatchTable *table );
DECLEXPORT(void) crUnpackPush(void);
DECLEXPORT(void) crUnpackPop(void);

extern CRNetworkPointer * return_ptr;
extern CRNetworkPointer * writeback_ptr;

#if defined(LINUX) || defined(WINDOWS)
#define CR_UNALIGNED_ACCESS_OKAY
#else
#undef CR_UNALIGNED_ACCESS_OKAY
#endif
DECLEXPORT(double) crReadUnalignedDouble( const void *buffer );

#define READ_DATA( offset, type ) \
    *( (const type *) (cr_unpackData + (offset)))

#ifdef CR_UNALIGNED_ACCESS_OKAY
#define READ_DOUBLE( offset ) \
    READ_DATA( offset, GLdouble )
#else
#define READ_DOUBLE( offset ) \
    crReadUnalignedDouble( cr_unpackData + (offset) )
#endif

#define READ_NETWORK_POINTER( offset ) \
    ( cr_unpackData + (offset) )

/* XXX make this const */
#define DATA_POINTER( offset, type ) \
    ( (type *) (cr_unpackData + (offset)) )

#define INCR_DATA_PTR( delta ) \
    cr_unpackData += (delta)

#define INCR_DATA_PTR_NO_ARGS() \
    INCR_DATA_PTR( 4 )

#define INCR_VAR_PTR() \
    INCR_DATA_PTR( *((int *) cr_unpackData ) )

#define SET_RETURN_PTR( offset ) \
    crMemcpy( return_ptr, cr_unpackData + (offset), sizeof( *return_ptr ) );

#define SET_WRITEBACK_PTR( offset ) \
    crMemcpy( writeback_ptr, cr_unpackData + (offset), sizeof( *writeback_ptr ) );

#ifdef __cplusplus
}
#endif

#endif /* CR_UNPACK_H */
