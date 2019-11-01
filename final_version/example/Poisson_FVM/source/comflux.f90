subroutine comflux(U_value ,flux ,add_value,&
                    cell_coor,&
                    node_coor,&
                    eac_extent,&
                    eac_index,&
                    ean_extent,&
                    ean_index,&
                    nac_extent,&
                    nac_index,&
                    num_edges,&
                    num_cells,&
                    num_nodes)
implicit none
integer num_edges
integer num_cells
integer num_nodes

integer eac_extent(0:num_edges)
integer eac_index(0:eac_extent(num_edges)-1)
integer ean_extent(0:num_edges)
integer ean_index(0:ean_extent(num_edges)-1)
integer nac_extent(0:num_nodes)
integer nac_index(0:nac_extent(num_nodes)-1)

double precision U_value(0:num_cells-1)
double precision flux(0:num_edges-1)
double precision add_value(0:num_edges-1)
double precision cell_coor(0:2*num_cells-1)
double precision node_coor(0:2*num_nodes-1)

integer i,j
integer cell1,cell2,node1,node2
double precision length_cell,length_edge
double precision add_value1,add_value2
double precision edge_normal_x,edge_normal_y
double precision normal_x,normal_y,cellvec_x,cellvec_y
double precision bound_u(0:3)
logical TF
!边界温度
bound_u(0) = 0.0d0
bound_u(1) = 0.0d0
bound_u(2) = 0.0d0
bound_u(3) = 0.0d0
!通量表达式的系数求解，分别为f1（flux）和add（add_value）
DO i =0,num_edges-1
    flux(i)=0.0d0
    add_value(i)=0.0d0
ENDDO

DO i = 0,num_edges-1
    add_value1 =0.0d0
    add_value2 =0.0d0
    TF = .TRUE.

    node1 = ean_index(ean_extent(i))
    node2 = ean_index(ean_extent(i)+1)
    length_edge = sqrt((node_coor(2*node1)-node_coor(2*node2))**2+(node_coor(2*node1+1)-node_coor(2*node2+1))**2)
    normal_x = -(node_coor(2*node2+1)-node_coor(2*node1+1))/length_edge
    normal_y = (node_coor(2*node2)-node_coor(2*node1))/length_edge
    IF(eac_extent(i+1)-eac_extent(i) .eq. 2)THEN
        cell1 = eac_index(eac_extent(i))
        cell2 = eac_index(eac_extent(i)+1)
        length_cell = sqrt((cell_coor(2*cell1)-cell_coor(2*cell2))**2+(cell_coor(2*cell1+1)-cell_coor(2*cell2+1))**2)
        cellvec_x = (cell_coor(2*cell2)-cell_coor(2*cell1))/length_cell
        cellvec_y = (cell_coor(2*cell2+1)-cell_coor(2*cell1+1))/length_cell
        IF(((node_coor(2*node1)-cell_coor(2*cell1))*normal_x+(node_coor(2*node1+1)-cell_coor(2*cell1+1))*normal_y) .lt. 0.0D0) THEN
            normal_x = -normal_x
            normal_y = -normal_y
            TF = .FALSE.
        ENDIF
        
        flux(i)=length_edge/((normal_x*cellvec_x+normal_y*cellvec_y)*length_cell)*(U_value(cell2)-U_value(cell1))
        edge_normal_x = -normal_y
        edge_normal_y = normal_x
        DO j=nac_extent(node1),nac_extent(node1+1)-1
            add_value1 = add_value1+U_value(nac_index(j))/(nac_extent(node1+1)-nac_extent(node1))
        ENDDO

        DO j=nac_extent(node2),nac_extent(node2+1)-1
            add_value2 = add_value2+U_value(nac_index(j))/(nac_extent(node2+1)-nac_extent(node2))
        ENDDO

        IF(TF)THEN
            add_value(i)=-(cellvec_x*edge_normal_x+cellvec_y*edge_normal_y)*   &
            (add_value2-add_value1)/(cellvec_x*normal_x+cellvec_y*normal_y)
        ELSEIF(.NOT. TF) THEN
            add_value(i)=-(cellvec_x*edge_normal_x+cellvec_y*edge_normal_y)*   &
            (add_value1-add_value2)/(cellvec_x*normal_x+cellvec_y*normal_y)
        ENDIF
    ELSEIF(eac_extent(i+1)-eac_extent(i) .eq. 1) THEN
        cell1 = eac_index(eac_extent(i))
        IF(dabs(normal_x) .eq. 1.0D0 .AND. cell_coor(2*cell1) .gt. node_coor(2*node1))THEN
            flux(i) = length_edge/cell_coor(2*cell1)*(bound_u(0)-U_value(cell1))
        ELSEIF (dabs(normal_x) .eq. 1.0D0 .AND. cell_coor(2*cell1) .lt. node_coor(2*node1))THEN
            flux(i) = length_edge/(1-cell_coor(2*cell1))*(bound_u(1)-U_value(cell1))
        ELSEIF(dabs(normal_y) .eq. 1.0D0 .AND. cell_coor(2*cell1+1) .gt. node_coor(2*node1+1))THEN
            flux(i) = length_edge/cell_coor(2*cell1+1)*(bound_u(2)-U_value(cell1))
        ELSEIF(dabs(normal_y) .eq. 1.0D0 .AND. cell_coor(2*cell1+1) .lt. node_coor(2*node1+1))THEN
            flux(i) = length_edge/(1-cell_coor(2*cell1+1))*(bound_u(3)-U_value(cell1))
        ENDIF
        add_value(i)=0.0d0
    ELSE
        write(6,*)"Error:wrong number of cells neighbouring to edge"
        stop
    ENDIF
    
ENDDO


return
end subroutine comflux