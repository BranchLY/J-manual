subroutine computeu(u_current,&
                    u_new,&
                    flux,&
                    add_value,&
                    cell_coords,&
                    node_coords,&
                    eac_extent,&
                    eac_index,&
                    cae_extent,&
                    cae_index,&
                    can_extent,&
                    can_index,&
                    num_cells,&
                    num_edges,&
                    dt)
implicit none
integer num_cells
integer num_edges
integer eac_extent(0:num_edges-1),eac_index(0:eac_extent(num_edges)-1)
integer cae_extent(0:num_cells-1),cae_index(0:cae_extent(num_cells)-1)
integer can_extent(0:num_cells-1),can_index(0:cae_extent(num_cells)-1)

double precision u_current(0:num_cells-1),u_new(0:num_cells-1)
double precision flux(0:num_edges-1),add_value(0:num_edges-1)
double precision cell_coords(0:2*num_cells-1),node_coords(0:2*num_edges-1)
double precision dt

integer i,j
integer edge,cell
integer node1,node2,node3
double precision S,source,flux_add
!计算守衡量u的新值（u_new)
DO i=0,num_cells-1
    u_new(i)=0.0D0
    flux_add= 0.0D0
    node1 = can_index(can_extent(i))
    node2 = can_index(can_extent(i)+1)
    node3 = can_index(can_extent(i)+2)
    S = dabs(0.5D0*((node_coords(2*node2)-node_coords(2*node1))*(node_coords(2*node3+1)-node_coords(2*node1+1))&
    -(node_coords(2*node2+1)-node_coords(2*node1+1))*(node_coords(2*node3)-node_coords(2*node1))))

    source = -2*(cell_coords(2*i+1)**2-cell_coords(2*i+1)+cell_coords(2*i)**2-cell_coords(2*i))

    DO j=0,cae_extent(i+1)-cae_extent(i)-1
        edge = cae_index(cae_extent(i)+j)
        cell = eac_index(eac_extent(edge))
        IF(cell==i)THEN
            flux_add=flux_add+flux(edge)*dt+add_value(edge)*dt
        ELSE
            flux_add=flux_add-flux(edge)*dt-add_value(edge)*dt
        ENDIF
    ENDDO
    u_new(i)=(u_new(i)+flux_add+source*dt*S+u_current(i)*S)/S
    
ENDDO
RETURN
END subroutine computeu