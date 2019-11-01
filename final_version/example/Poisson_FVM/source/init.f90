subroutine init(U_value,num_cells)
implicit none
integer num_cells
double precision U_value(0:num_cells-1)

integer i
!current守衡量u赋初值为0
DO i=0,num_cells-1
    U_value = 0.0D0
ENDDO

RETURN
END subroutine init