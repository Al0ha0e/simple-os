.text
.globl _entry
_entry:
    lui		$t1, 0xa000		# $t1 =0xa00003f8
    ori	    $t1, $t1, 0x03f8 
    #  WriteReg(IER, 0x00);
    li		$t2, 0x00		# $t2 =0x00 
    sb		$t2, 1($t1)		# IER = 0
    # WriteReg(LCR, LCR_BAUD_LATCH);
    li		$t2, 0x080		# $t2 =0x00 
    sb		$t2, 3($t1)		# LCR = 1<<7
    # WriteReg(0, 0x03);
    li      $t2,0x03
    sb		$t2, 0($t1)		# 
    # WriteReg(1, 0x00);
    li		$t2, 0x00		# $t2 =0x00 
    sb		$t2, 1($t1)		# IER = 0
    # WriteReg(LCR, LCR_EIGHT_BITS);
    li		$t2, 0x03		# $t2 =0x00 
    sb		$t2, 3($t1)		# IER = 0
    # WriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);
    li		$t2, 0x07		# $t2 =0x00 
    sb		$t2, 2($t1)		# IER = 0
    # WriteReg(IER, IER_TX_ENABLE | IER_RX_ENABLE);
    li		$t2, 0x03		# $t2 =0x00 
    sb		$t2, 1($t1)		# IER = 0
    #output A
    li		$t2, 0x041		# $t2 =0x00 
    sb		$t2, 0($t1)		# IER = 0
    
    
    