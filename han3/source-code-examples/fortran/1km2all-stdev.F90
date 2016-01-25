! This program compute stdev of 1-km elev for other resolutions 
! 
! Usage:
! 1km2all-stdev to-resolution(km) output_file 

! Usage example, to get 10km stdev data: 
! 1km2all-stdev 10 stdev_10KM.1gd4r 



	program toall
 
        implicit NONE
        integer iargc, i, j, i1, j1, nrec, grec
	integer itype, ires, nw
	character*80 inputf, outputf, maskf, vegf
        character*80 ctmp

	! data structures
	integer, parameter :: ic = 36000, ir=15000  !input grid
	integer, parameter :: nt = 1    ! number of veg types 
	integer :: nc, nr   ! output grid
	integer :: iveg, maxi, maxj, mini, minj
        integer :: cnt, ix
	real, allocatable :: in(:, :), out(:), tmpin(:, :) 
	real ::  sumx2, sumx, x 

        i =  iargc()
        If (i.ne.2) Then   ! wrong cmd line args, print usage
	  write(*, *)"Usage:"
	  write(*, *)"1km2all-stdev out-res(km) output_file"
	  write(*, *)"Usage example: "
	  write(*, *)"1km2all-stdev 5 stdev_5KM.1gd4r"
	  stop
        End If

       call getarg(1, ctmp)
       read(ctmp, *) ires

       call getarg(2, outputf)

       ! for now do not support weird resolutions
       if (mod(ic, ires) .NE. 0 .or. mod(ir, ires) .NE. 0) then
         write(*, *) "Resolution ", ires, "km is not supported yet." 
         stop
       end if
       nc = nint(ic / ires * 1.0) 
       nr = nint(ir / ires * 1.0) 
       if (nc*ires .LT. ic ) nc = nc + 1
       if (nr*ires .LT. ir ) nr = nr + 1
       write(*, *) "nc=",  nc, " nr=",  nr

      ! how wide is the input band 
        nw = ires
        if (ires.eq.1) nw=3   !special handling of 1km resolution, 3x3 box

	allocate(in(ic, nw))
	allocate(tmpin(ic, nw))
	allocate(out(nc))

	open(51, file="/lisraid_bak/PARAMS/1KM/ELEVATION/lis_elev.1gd4r", &
          status='old', form='unformatted', &
          access='direct',recl=ic*ires*4)

	open(52, file=outputf, form='unformatted', &
          access='direct',recl=nc*4)

       if (ires.eq.1)  then    !special handling of 1km resolution, 3x3 box

        Do j=1, nr      !
          if (j .eq. 1) then 
           in(:, 1) = -9999.0 
           read(51, rec=j) in(:, 2) 
           read(51, rec=j+1) in(:, 3) 
          elseif ( j .eq. nr ) then 
           read(51, rec=j-1) in(:, 1) 
           read(51, rec=j) in(:, 2) 
           in(:, 3) = -9999.0 
          else
           in(:, 1)  = tmpin(:, 2) 
           in(:, 2)  = tmpin(:, 3) 
           read(51, rec=j+1) in(:, 3) 
          end if 

          tmpin = in 
           
          Do i = 1, nc
            mini = i - 1
            maxi = i + 1 

            minj = 1
            maxj = nw

            cnt = 0
            sumx2 = 0.0
            sumx = 0.0

            Do j1 = minj, maxj     !now compute stdev of the nw x nw box
             Do i1 = mini, maxi
               ! wrap E-W edges 
                ix = i1 
                if (ix .LE. 0) ix = nc + ix
                if (ix .GT. nc) ix = ix - nc 

                x = in(ix, j1)
              if ( x .NE. -9999.0) then
                sumx = sumx + x
                sumx2 = sumx2 + x* x
                cnt = cnt + 1
              end if
             End Do ! i1
            End Do   ! j1

            If (cnt .GT. 1 ) then
            !http://en.wikipedia.org/wiki/Standard_deviation
              out(i) = sqrt( ( cnt*sumx2 - sumx*sumx )/(cnt * (cnt-1)) )
            else
              out(i) = -9999.0
            end if

          End Do   ! i
          write(52, rec=j) out
         End Do   ! j


       else  !  > 1km resolutions, simpler 

	Do j=1, nr
           read(51, rec=j) in
          Do i = 1, nc
            mini = (i -1) * nw + 1
            maxi = min(ic, i*nw) 

            cnt = 0 
            sumx2 = 0.0
            sumx = 0.0
            Do j1 = 1, nw          !now compute stdev of the nw x nw box 
             Do i1 = mini, maxi
              x = in(i1, j1) 
              if ( x .NE. -9999.0) then 
                sumx = sumx + x 
                sumx2 = sumx2 + x* x 
                cnt = cnt + 1
              end if 
             End Do ! i1
            End Do   ! j1

            If (cnt .GT. 1 ) then 
            !http://en.wikipedia.org/wiki/Standard_deviation
              out(i) = sqrt( ( cnt*sumx2 - sumx*sumx )/(cnt * (cnt-1)) ) 
            else
              out(i) = -9999.0 
            end if

          End Do   ! i
          write(52, rec=j) out
         End Do   ! j

        end if 

        close(51) 
        close(52) 
	deallocate(in)
	deallocate(tmpin)
	deallocate(out)

	end 


