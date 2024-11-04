;---------------------------------------------------;
;													;
;	Assembler Programming Languages Gaussian Blur	;
;	Author:	Daniel Pasierb							;
;	INF SSI GLI										;
;	SEK 10											;
;													;
;													;
;---------------------------------------------------;

.data
	radius	dq 0
	ker_size dq	0;
	stddiv	dd	0.0	; standard deviation

	sum		dd	0.0 ;sum kernel
	sum_B	dd	0.0	;sum blue channel
	sum_G	dd	0.0	;sum green channel
	sum_R	dd	0.0	;sum red channel
	src		dq 0	;var to store src address


	img_width dq 0
	rel_img_width dq 0 ;img_width * 3 (3 channels)
	img_height dq 0
	rel_img_height dq 0 ;img_height * 3 (3 channels)

	;consts
	two		dd 2.0
	one		dd 1.0


	two_pi		dd 6.283185307179586  ; 2 * pi value for Gaussian formula

	temp dq 0               ; Reserve space for a temporary variable
	tempD dq 0.0               ; Reserve space for a temporary variable

	kernel DWORD 21 DUP(0.0)  ; Define an array of 21
	tablecount QWORD 0       ; A QWORD to store the number of data inputs (initially 0)


	blr_tab BYTE 24883200 DUP(?)
	blr_tab_size QWORD 0

.CODE	

exppow PROC
	MOVSS	XMM1, XMM0		;xmm0 is a value of x and xmm1 is a value of x^n 

	mov		r10, 1

	cvtsi2ss XMM2, r10		
	ADDSS	XMM2, XMM1		;starting value and place that resault will be kept

	xor		r10, r10		;loop counter
	mov		r11, 1			;factorial current 
	mov		r12, 1			;factorial next num
	
	calc_exp_loop:
		cmp r10, 10
		jge proc_end

		MULSS XMM1, XMM0		;calc x^n
		MOVSS XMM4, XMM1

		inc r12					;mov fac next num by 1
		imul r11, r12			;mul the fac curr with fac next num to get n!
	
		cvtsi2ss xmm3, r11		;convert to float

		divss xmm4, xmm3

		addss xmm2, xmm4

		inc r10                       ; increment loop counter
		jmp calc_exp_loop


	proc_end:
	movss xmm0, xmm2
    ret                           ; Return from the function
exppow ENDP

calcuGaussKernel1DASM PROC 
	;initialize

	push	rbp
	push	rsp

	xorps	xmm0, xmm0
	movss 	sum, xmm0
	mov 	ker_size, 0
	mov     tablecount, 0

	;calcu size 2rad+1= kernel size
	MOV		RAX, RCX
	SHL		RAX, 1			;*2
	ADD		RAX, 1
	MOV		ker_size, RAX	;load to var ker_size

	;cakcu stddiv rad/2
	MOV		RAX, RCX
	cvtsi2ss xmm0, rax
    movss	xmm1, two
	divss	xmm0, xmm1		;/2
	movss	stddiv, xmm0	;SAVE TO STDDIV

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

					; Calculate x * x
		mov		r9, rax
		imul	r9, rax				;x^2

		movss 	xmm3, stddiv
		mulss	xmm3, xmm3
		addss   xmm3, xmm3			;2*stddiv^2

		cvtsi2ss xmm2, r9

		divss	xmm2, xmm3			;x^2/2*stddiv^2
		movss	xmm0, xmm2			

		call    exppow				

		movss	xmm1, one 
		divss	xmm1, xmm0
		movss	xmm0, xmm1			; Result is in xmm0 (top of 1d gauss distrb)

		movss	xmm1, two_pi		; 2 * pi
		movss	xmm2, stddiv		; stddiv
		mulss	xmm2, xmm2			; stddiv^2
		mulss	xmm1, xmm2			
		sqrtss	xmm1, xmm1			;bottom of a 1d gauss distrb

		divss	xmm0, xmm1			;resault of 1d gauss distrb for x

							;move to tab
							;check if theres is space
		mov		rax, tablecount					; Load the current count into rax
		cmp		rax, 21					        ; Check if we've reached the maximum count (20)
		jge		end_program				        ; If table is full, jump to end


        lea		rcx, kernel				       ; Load the address of the table
        mov		rsi, rax				       ; Use rax as the index for the table
        shl		rsi, 2							; Multiply the index by  for correct byte address
        add		rcx, rsi                      ; Get the address of table[index]
		movss	xmm1, xmm0                   
        movss	dword ptr [rcx], xmm1                 ; Move the computed value from xmm0 to table[index]

		movss	xmm0, sum
		addss	xmm0, xmm1
		movss	sum, xmm0

        inc		tablecount                    ; Increment the count of elements in the table

        ; Increment loop index
        inc		rdi                           ; Move to the next index
		cmp		rdi, ker_size				; Check if we have computed enough terms
		jl		calc_kernel_loop 
		
	;end loop 
	xor		rdi, rdi
	movss	xmm1, sum 

	norm_loop:

		lea		rcx, kernel

		mov		rsi, rdi                      
		shl		rsi, 2                        
		add		rcx, rsi	;move to correct offset
		
		movss	xmm0, dword ptr [rcx] 
		divss	xmm0, xmm1
		movss	dword ptr [rcx], xmm0

		inc		rdi                           
		cmp		rdi, ker_size
		jl		norm_loop

	end_program:
	lea		rax, kernel
	pop		rsp
	pop		rbp
			ret
calcuGaussKernel1DASM ENDP


GaussianBlurHorizontalASM PROC
	; rcx	'src' 
	; rdx	'kernel' 
	; r8	'img_width' 
	; r9	'img_height' 
	; stack	'radius' - also in .data section

	push	rbp
	push	rsp 

	mov img_width, r8
	mov img_height, r9

	mov		r8, img_width
	imul	r8, 3
	mov		rel_img_width, r8

	mov		r9, img_height
	imul	r9, 3
	mov		rel_img_height, r9

	mov		src, rcx
	lea		r15, blr_tab	; destination address
	xor		rdi, rdi		; image y val = 0

	height_loop:

		mov		rcx, src		;load src
		lea		r15, blr_tab	;load dest

		mov		rsi, rdi
		imul	rsi, img_width
		imul 	rsi, 3			;calc y offset

		add		r15, rsi		; move to correct y position in destination
		add		rcx, rsi		; move to correct y position in source
			
		xor		r10, r10		; image x val = 0 (iterator)

			width_loop:
				
				mov		r11, r10
				imul 	r11, 3	;calc x offset

				add 	r15, r11	; move to correct x position in destination
				add		rcx, r11	; move to correct x position in source

				xor 	r8, r8		;index of pixel in processing
				xor		r9, r9		;kernel table
				xor 	r13, r13	;iterator for kernel
				sub		r13, radius	;start from the left edge of the kernel

				; RCX - pixel pos in source 
				; R15 - pixel pos in destination
				apply_kernel_loop:
					mov		r8, rcx		;mov pos x
					mov 	r9, r13		;mov iterator

					imul	r9, 3	;calc pixel offset
					sub		r8, r9	;pos x + k
					
					mov 	rax, rcx
					sub 	rax, r11	;rax - start of the line
					
					cmp     r8, rax		;is kernel on the line
					jl		clampMIN	;if False clamp to min  
					jmp     contToMax	;if True check the max
					

					clampMIN:
						mov		r8, rax	;clamp val to the first in line
						jmp     cont	;if min was out of range there is no need to check max, continue

					contToMax:
						mov		rax, rel_img_width	
						add		rax, rcx
						sub 	rax, r11	; last val in the line

						cmp     r8, rax		;is the kernel outside
						jge     clampMAX	;True clamp the val to the last in line
						jmp     cont        ;False continue 

					clampMAX:
						mov		r8, rax		; set to the end of the line
						sub		r8, 3		; last value 

					cont:

					MOV    R14, r8
					

					mov		al, byte ptr [r8]		;read Blue from source
					mov		bl, byte ptr [r8+1]		;read Green from source
					mov		r12b, byte ptr [r8+2]	;read Red from source

					;move to correct offset in kernel
					lea     r8, kernel	;load kernel

					mov		r9, r13	
					add		r9, radius
					shl 	r9, 2		;iterator*4 = kernel tab offset

					add		r8, r9		; add offset
					movss	xmm0, dword ptr [r8]	;read kernel value
					;multiply pixel by kernel

					;__________________BLUE____________________
					movzx eax, al			
					cvtsi2ss xmm1, eax

					mulss xmm1, xmm0       
					movss xmm2, sum_B        ; Load the float sum_B into 
					addss xmm2, xmm1         ; xmm2 = xmm2 + xmm1
					movss sum_B, xmm2        ; Store the new sum_B value              


					;__________________GREEN____________________
					movzx ebx, bl				
					cvtsi2ss xmm1, ebx				

					mulss xmm1, xmm0       
					movss xmm2, sum_G        ; Load the float sum_B into 
					addss xmm2, xmm1		 ; xmm2 = xmm2 + xmm1
					movss sum_G, xmm2        ; Store the new sum_B value                 

					;__________________RED____________________
					movzx r12d, r12b				
					cvtsi2ss xmm1, r12d				

					mulss xmm1, xmm0       
					movss xmm2, sum_R        ; Load the float sum_B into 
					addss xmm2, xmm1           ; xmm2 = xmm2 + xmm1
					movss sum_R, xmm2        ; Store the new sum_B value      

					inc r13
					cmp r13, radius
					jle apply_kernel_loop

				;_____________MOVE_TO_DEST_______________
				movss	xmm0, sum_B      ; Load sum_B into xmm0
				movss	xmm1, sum_G      ; Load sum_G into xmm1
				movss	xmm2, sum_R      ; Load sum_R into xmm2

				roundss xmm0, xmm0, 0 
				cvttss2si rax, xmm0     
				mov sum_B, 0   
				
				roundss xmm1, xmm1, 0 
				cvttss2si rbx, xmm1    
				mov sum_G, 0  
				
				roundss xmm2, xmm2, 0 
				cvttss2si r12, xmm2     
				mov sum_R, 0          

				mov 	byte ptr [r15], al		;write Blue to destination
				mov 	byte ptr [r15+1], bl		;write Green to destination
				mov 	byte ptr [r15+2], r12b	;write Red to destination

				sub 	rcx, r11	;move source back to line start
				sub 	r15, r11	;move destination back to line start

				inc		r10 

				cmp		r10, img_width
				jl		width_loop

		inc		rdi
		cmp		rdi, img_height
		jl		height_loop
;____________________________________________________
;move dest to src

	xor		rdi, rdi	;reset index
		
	mov		rsi, img_width	
	imul	rsi, img_height
	imul	rsi, 3		
	sub 	rsi, 3		;calc tab size - tab is indexed from 0

	mov 	blr_tab_size, rsi

	mov_data_loop:


		lea		r15, blr_tab	; destination address
		mov		rcx, src		;load src address

		add		rcx, rdi    ;move src to correct offset
		add		r15, rdi	;move dest to correct offset
		
		mov		al, byte ptr [r15]		;read Blue from dest
		mov		bl, byte ptr [r15+1]	;read Green from dest
		mov		r12b, byte ptr [r15+2]	;read Red from dest

		mov 	byte ptr [rcx], al			;write Blue to src
		mov 	byte ptr [rcx+1], bl		;write Green to src
		mov 	byte ptr [rcx+2], r12b		;write Red to src

		add		rdi, 3
		cmp		rdi, blr_tab_size
		jl		mov_data_loop

	pop		rsp
	pop		rbp
	ret
GaussianBlurHorizontalASM ENDP


GaussianBlurVerticalASM PROC
	; rcx	'src' 
	; rdx	'kernel' 
	; r8	'img_width' 
	; r9	'img_height' 
	; stack	'radius' - also in .data section

	push	rbp
	push	rsp 

	mov img_width, r8
	mov img_height, r9

	mov		r8, img_width
	imul	r8, 3
	mov		rel_img_width, r8

	mov		r9, img_height
	imul	r9, 3
	mov		rel_img_height, r9

	mov		src, rcx
	lea		r15, blr_tab	; destination address
	xor		rdi, rdi		; image y val = 0

	height_loop:

		mov		rcx, src		;load src
		lea		r15, blr_tab	;load dest

		mov		rsi, rdi
		imul	rsi, img_width
		imul 	rsi, 3			;calc y offset

		add		r15, rsi		; move to correct y position in destination
		add		rcx, rsi		; move to correct y position in source
			
		xor		r10, r10		; image x val = 0 (iterator)

			width_loop:
				
				mov		r11, r10
				imul 	r11, 3	;calc x offset

				add 	r15, r11	; move to correct x position in destination
				add		rcx, r11	; move to correct x position in source

				xor 	r8, r8		;index of pixel in processing
				xor		r9, r9		;kernel table
				xor 	r13, r13	;iterator for kernel
				sub		r13, radius	;start from the left edge of the kernel

				; RCX - pixel pos in source 
				; R15 - pixel pos in destination
				apply_kernel_loop:
					mov		r8, rcx		;mov pos x
					mov 	r9, r13		;mov iterator

					imul	r9, rel_img_width ;iterator times img width = pixel offset


					add		r8, r9	;pos x + k	;pos + offset
					
					mov 	rax, src
					
					cmp     r8, rax		;is kernel in the img
					jl		clampMIN	;if False clamp to min  
					jmp     contToMax	;if True check the max
					

					clampMIN:
						mov		r9, src
						add 	r9, r11 ;set the pixel value to the start of the img + x offset 
						mov		r8, r9
						jmp     cont	;if min was out of range there is no need to check max, continue

					contToMax:
						mov		rax, img_width
						imul	rax, img_height
						imul	rax, 3
						add		rax, src
						sub 	rax, 3		; last val in the img

						cmp     r8, rax		;is the kernel outside
						jge     clampMAX	;True clamp the val to the last in line
						jmp     cont        ;False continue 

					clampMAX:
						mov		r9, rax
						add     r9, 3		;end of img
						sub		r9, rel_img_width
						add 	r9, r11		;start of last img line + x offset

						mov		r8, r9		; set to the end of the line
					cont:

					MOV    R14, r8

					mov		al, byte ptr [r8]		;read Blue from source
					mov		bl, byte ptr [r8+1]		;read Green from source
					mov		r12b, byte ptr [r8+2]	;read Red from source

					;move to correct offset in kernel
					lea     r8, kernel	;load kernel

					mov		r9, r13	
					add		r9, radius
					shl 	r9, 2		;iterator*4 = kernel tab offset

					add		r8, r9		; add offset
					movss	xmm0, dword ptr [r8]	;read kernel value
					;multiply pixel by kernel

					;__________________BLUE____________________
					movzx eax, al			
					cvtsi2ss xmm1, eax

					mulss xmm1, xmm0       
					movss xmm2, sum_B        ; Load the float sum_B into 
					addss xmm2, xmm1         ; xmm2 = xmm2 + xmm1
					movss sum_B, xmm2        ; Store the new sum_B value              


					;__________________GREEN____________________
					movzx ebx, bl				
					cvtsi2ss xmm1, ebx				

					mulss xmm1, xmm0       
					movss xmm2, sum_G        ; Load the float sum_B into 
					addss xmm2, xmm1		 ; xmm2 = xmm2 + xmm1
					movss sum_G, xmm2        ; Store the new sum_B value                 

					;__________________RED____________________
					movzx r12d, r12b				
					cvtsi2ss xmm1, r12d				

					mulss xmm1, xmm0       
					movss xmm2, sum_R        ; Load the float sum_B into 
					addss xmm2, xmm1           ; xmm2 = xmm2 + xmm1
					movss sum_R, xmm2        ; Store the new sum_B value      

					inc r13
					cmp r13, radius
					jle apply_kernel_loop

				;_____________MOVE_TO_DEST_______________
				movss	xmm0, sum_B      ; Load sum_B into xmm0
				movss	xmm1, sum_G      ; Load sum_G into xmm1
				movss	xmm2, sum_R      ; Load sum_R into xmm2

				roundss xmm0, xmm0, 0 
				cvttss2si rax, xmm0     
				mov sum_B, 0   
				
				roundss xmm1, xmm1, 0 
				cvttss2si rbx, xmm1    
				mov sum_G, 0  
				
				roundss xmm2, xmm2, 0 
				cvttss2si r12, xmm2     
				mov sum_R, 0          

				mov 	byte ptr [r15], al		;write Blue to destination
				mov 	byte ptr [r15+1], bl		;write Green to destination
				mov 	byte ptr [r15+2], r12b	;write Red to destination

				sub 	rcx, r11	;move source back to line start
				sub 	r15, r11	;move destination back to line start

				inc		r10 

				cmp		r10, img_width
				jl		width_loop

		inc		rdi
		cmp		rdi, img_height
		jl		height_loop
;____________________________________________________
;move dest to src

	xor		rdi, rdi	;reset index
		
	mov		rsi, img_width	
	imul	rsi, img_height
	imul	rsi, 3		
	sub 	rsi, 3		;calc tab size - tab is indexed from 0

	mov 	blr_tab_size, rsi

	mov_data_loop:


		lea		r15, blr_tab	; destination address
		mov		rcx, src		;load src address

		add		rcx, rdi    ;move src to correct offset
		add		r15, rdi	;move dest to correct offset
		
		mov		al, byte ptr [r15]		;read Blue from dest
		mov		bl, byte ptr [r15+1]	;read Green from dest
		mov		r12b, byte ptr [r15+2]	;read Red from dest

		mov 	byte ptr [rcx], al			;write Blue to src
		mov 	byte ptr [rcx+1], bl		;write Green to src
		mov 	byte ptr [rcx+2], r12b		;write Red to src

		add		rdi, 3
		cmp		rdi, blr_tab_size
		jl		mov_data_loop

	pop		rsp
	pop		rbp
	ret
GaussianBlurVerticalASM ENDP
END