# cmake -DDLLS=<list> -DDEST=<dir> -P CopyDlls.cmake: copies each DLL in DLLS (which may be empty) into DEST.
foreach( dll IN LISTS DLLS )
  file( COPY "${dll}" DESTINATION "${DEST}" )
endforeach()
