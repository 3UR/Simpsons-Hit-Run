# (C) 2001 Radical Entertaiment
#
# Function to call a Ps2 virtual function
#
# This function is limited to passing up to 7 ints and 8 floats
# (one int is taken by the this pointer)
# 
# Author: Tim Hinds

#ifndef mips_regdef_h

#define zero	$0
#define v0	$2
#define v1	$3
#define a0	$4
#define a1	$5
#define a2	$6
#define a3	$7
#define t0	$8
#define t1	$9
#define t2	$10
#define t3	$11
#define t4	$12
#define t5	$13
#define t6	$14
#define t7	$15
#define s0	$16
#define s1	$17
#define s2	$18
#define s3	$19
#define s4	$20
#define s5	$21
#define s6	$22
#define s7	$23
#define t8	$24
#define t9	$25
#define k0	$26
#define k1	$27
#define gp	$gp
#define sp	$sp
#define fp  $30
#define s8	$30
#define ra	$31
#define pc	$pc

#endif /* mips_regdef_h */
.text
.global	InvokeVf

#
# InvokeVf( void * pObject, unsigned int vtableoffset, void * pParams, void * pFloatParams )
#             a0                    a1                       a2              a3
#

InvokeVf:
	addiu sp, sp, 0xFFE0	# set up the stack frame
    sd ra, 0x0010(sp)       # save the return address because we call out
    sd fp, 0x0000(sp)       # save the callers frame pointer

	move fp, sp		        # sony has a crazy frame setup, its just

	move  t4, a2	        # t1 holds pParams
	move  t5, a3		    # t2 holds pFloatParams

	# calculate the address into the vtable.  Shittily, gcc puts the vtable
	# pointer at the END of the class so this will only work for pure
	# abstract classes which have no data.  Otherwise we need to know the
	# size of the class which is practically impossible.							

	# a0 holds "that", so for abstract class that points to pointer to vtble,
	# so get the vtable

	lw	 t6, 0x0000(a0)   # load the vtable
    sll  a1, a1, 0x03     # a1 holds the function number so multiply by 8 to convert to dwords
	add  t6, t6, a1	      # t6 now holds the vtable entry address
	lw   t7, 0x0000(t6)   # first entry is the this fixup
	add  a0, a0, t7	      # .. fixup this          
	lw   t6, 0x0004(t6)   # .. get the funciton address

	lw    a1, 0x0000(t4)  # load all of the simple (int) params
	lw    a2, 0x0004(t4)
	lw    a3, 0x0008(t4)
	lw    t0, 0x000C(t4)
	lw    t1, 0x0010(t4) 
	lw    t2, 0x0014(t4)
	lw    t3, 0x0018(t4)

	lwc1  $f12, 0x0000(t5) # load all of the float params
	lwc1  $f13, 0x0004(t5)
	lwc1  $f14, 0x0008(t5)
	lwc1  $f15, 0x000C(t5)
	lwc1  $f16, 0x0010(t5)
	lwc1  $f17, 0x0014(t5)
	lwc1  $f18, 0x0018(t5)
	lwc1  $f19, 0x001C(t5)

	jalr  t6				# call the function

	move sp, fp			    
	ld ra, 0x0010(sp)
	ld fp, 0x0000(sp)
	addiu sp, sp, 0x0020
	jr ra
