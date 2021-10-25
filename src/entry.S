.text
.globl _entry
_entry:
    la		sp, stack0		#
    li		t1, 4096		# $t1 =4096 		
    add		sp, sp, t1	# $sp = sp1 +t1t2
    call    start

spin:
    j spin
    

    