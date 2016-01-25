! This program aggregates 3hrly PERSIANN  data into daily precip data 
! starting from 12Z of previous day to 12Z of present day, 
! Persiann 3hr is the accumulation at the beginning of the period
! i.e.,  for day1: 
! day1 = day0@12Z + day0@15Z + day0@18Z + day0@21Z 
!      + day1@0Z + day1@3Z + day1@6Z + day1@9Z 
! to follow Higgins daily data convention.
! missing data handling: if all 8 files for a day are missing, 
!  the daily is missing; if at least one has data, scale the 
!   available data to daily 
! usage: aggr2daily 

! starging from 2/1/2003 to 10/31/2007 GMT
!  gmtsec: 1044057600 to 1193788800
!
	Program aggr2
	implicit NONE
        integer, parameter :: sec0=1044057600
        integer, parameter :: sec1=1193788800
        integer, parameter :: dsec=24*60*60
        integer isec 

       Do isec=sec0, sec1, dsec 
       
         call faggr(isec) 

       End Do
        
       stop 
       end 


       subroutine faggr(isec) 
        integer, parameter :: hr3 = 3*60*60 
	integer, parameter :: nc=1440, nr=480, nf=8  
	real aggr(nc, nr), input(nc, nr)
	integer icnt(nc, nr)
	integer :: it, isec
        character*100 filenm, dailyf

       
        icnt = 0 
        aggr = 0.0

      call getdailyfn(isec, dailyf)   !get daily data file name 

      Do it = isec - 4*hr3, isec + 3*hr3, hr3

         call getfilename(it, filenm)   ! 3-hrly file name 
         write(*, *) "Input file: ", filenm 
         open(19, file=filenm, form="unformatted", access="direct", &
          recl=nc*nr*4, status="old",  iostat=ios)

         if(ios.ne.0) then
           write(*, *)"skipping ", filenm 
           close(19)
          else
           read(19, rec=1) input
           close(19)

          Do j=1, nr
            Do i=1, nc
             If(input(i, j).GE.0.0.AND.input(i, j).LE.10000) then   !* skip undef & nan 
              aggr(i, j) = aggr(i, j) + input(i, j)   !with weight
              icnt(i, j) = icnt(i, j) + 1
             end if
            End Do
           End Do

         end if

        End Do  ! done it 

          Do j=1, nr
            Do i=1, nc
             If (icnt(i, j).EQ.0) then    ! fill with missing data
               aggr(i,  j) = -9999.0
             else 
               aggr(i, j) = aggr(i, j)/icnt(i, j)*8.0    !scale to 24-hr
             end if
            End do
          End do

        write(*, *) "Aggr'd: output file: ", dailyf

        open(20, file=dailyf, form="unformatted", access="direct", &
            recl=nc*nr*4)
          write(20, rec=1) aggr
        close(20)
     
        return 
        end 


      subroutine getdailyfn(isec, dailyf)   !get daily data file name 
        integer :: it, isec, iyr, iys, im, id, ih, seconds, tout(9)
        character*100 dailyf

        call gmtime(isec, tout)
        iyr = 1900 + tout(6)
        im = tout(5)+1
        id = tout(4)
        ih = tout(3)

        write(dailyf, 100) "/trmm2/PERSIANN/", iyr, im, &
           "/", iyr, im, id, "_daily.1gd4r" 
100 format(A, I4.4, I2.2, A, I4.4, I2.2, I2.2, A)

       return
       end


      subroutine getfilename(isec, filenm)   !get 3hrly data file name
        integer :: isec, iyr, iys, im, id, ih, seconds, tout(9)
        character*100 filenm

        call gmtime(isec, tout)
        iyr = 1900 + tout(6)
        im = tout(5)+1
        id = tout(4)
        ih = tout(3)

        write(filenm, 200) "/trmm2/PERSIANN/", iyr, im, &
           "/", iyr, im, id, ih,  ".bin"
200 format(A, I4.4, I2.2, A, I4.4, I2.2, I2.2, I2.2, A)

       return
       end



