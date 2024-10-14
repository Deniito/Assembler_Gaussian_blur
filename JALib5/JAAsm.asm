.data
	radius	dq 0
	ker_end	dq	0
	stddiv	REAL8	0.0
	sum		dd	0.0
	two		dq 2.0
	one		dq 1.0

	two_pi		dq 6.283185307179586  ; 2 * pi value for Gaussian formula
	e			dq 2.71828182846

	temp dq 0               ; Reserve space for a temporary variable
	tempD dq 0.0               ; Reserve space for a temporary variable

	table DWORD 21 DUP(0.0)  ; Define an array of 21 double-precision floating-point values, all initialized to 0.0
	tablecount QWORD 0       ; A DWORD to store the number of data inputs (initially 0)

	pvals	QWORD 7.38905609893 ; values of (sqrt(e)^1/sigm^2)
			QWORD 1.6487212707
			QWORD 1.248848869
			QWORD 1.13314845307
			QWORD 1.08328706767
			QWORD 1.05712774476
			QWORD 1.04166076253
			QWORD 1.0317434075
			QWORD 1.02499871407
			QWORD 1.02020134003

.CODE	

exppow PROC
	;xmm0 - exponent
	;e - base
	
	mov		r8, radius
	lea		rsi, pvals
	shl		r8, 3
	add		rsi, r8
	sub		rsi, 8 

	movq	xmm1, qword ptr [rsi]      ; Load the current double value into xmm0

	cvtsd2si r8, xmm0



    ; Initialize loop variables
    mov rax, 1                   ; loop counter for terms
	movsd	xmm0, xmm1				;keep initial val for multiplication

calc_exp_loop:
		; Calculate the next term: x^n / n!
		mulsd xmm1, xmm0            ; xmm2 = x^n (x^eax)
		; Prepare for the next iteration
		inc rax                       ; increment term counter

		; Check if we have reached a sufficient number of terms
		cmp rax, r8                   ; Check if we have computed enough terms
		jl calc_exp_loop              ; If not, continue the loop

		; Store the result in xmm0 (return the result)

	movsd xmm0, xmm1
    ret                           ; Return from the function
exppow ENDP






calcuGaussKernel1DASM PROC 
	;initialize

	push	rbp
	push	rsp

	;calcu size 2rad+1
	MOV		RAX, RCX
	SHL		RAX, 1			;*2
	ADD		RAX, 1
	MOV		ker_end, RAX	;load to var ker_end

	;cakcu stddiv rad/2f
	MOV		RAX, RCX
	cvtsi2sd xmm0, rax
    movsd	xmm1, two
	divsd	xmm0, xmm1		;/2
	movsd	stddiv, xmm0	;SAVE TO STDDIV


	;prep for loop
	mov		radius, RCX		;SAVE RAD
	xor		rcx, rcx		;RESET RCX
	xor		rdi, rdi		


	;firstloop
	calc_kernel_loop:
		
	;calcukernaltabloop
						; x = i - rad	
		mov     rax, rdi            ; rax = i
		sub     rax, radius			; rax = i - radius
		cvtsi2sd xmm0, rax          ; Convert x to double (xmm0)

					; Calculate x * x
		movapd  xmm1, xmm0          ; xmm1 = xmm0 (copy x)
		mulsd   xmm0, xmm0          ; xmm0 = x * x


		call    exppow				; Result is in xmm0 (top of 1d gauss distrb)

		movsd	xmm1, one 
		divsd	xmm1, xmm0
		movsd	xmm0, xmm1

		movsd	xmm1, two_pi
		movsd	xmm2, stddiv
		mulsd	xmm1, xmm2
		sqrtsd	xmm1, xmm1			;bottom of a 1d gauss distrb

		divsd	xmm0, xmm1



							;move to tab
							;check if theres is space
		mov		rax, tablecount					; Load the current count into rax
		cmp		rax, 20					        ; Check if we've reached the maximum count (20)
		jge		end_program				        ; If table is full, jump to end


        lea		rcx, table				       ; Load the address of the table
        mov		rsi, rax				       ; Use rax as the index for the table
        shl		rsi, 2							; Multiply the index by 8 (size of double) for correct byte address
        add		rcx, rsi                      ; Get the address of table[index]
		cvtsd2ss xmm1, xmm0 
        movss	dword ptr [rcx], xmm1                 ; Move the computed value from xmm0 to table[index]

		movss	xmm0, sum
		addss	xmm0, xmm1
		movss	sum, xmm0

        inc		tablecount                    ; Increment the count of elements in the table

        ; Increment loop index
        inc		rdi                           ; Move to the next index
		cmp		rdi, ker_end				; Check if we have computed enough terms
		jl		calc_kernel_loop 
		
	
	;end loop 
	xor		rdi, rdi
	movss	xmm1, sum 

	                    ; Load the address of the table


	norm_loop:

		lea		rcx, table
		mov		rsi, rdi                      
		shl		rsi, 2                        
		add		rcx, rsi                     
		movss	xmm0, dword ptr [rcx] 
		divss	xmm0, xmm1
		movss	dword ptr [rcx], xmm0

		inc		rdi                           
		cmp		rdi, ker_end
		jl		norm_loop

		


		
		
	end_program:
	lea		rax, table
	pop		rsp
	pop		rbp
			ret


calcuGaussKernel1DASM ENDP


END