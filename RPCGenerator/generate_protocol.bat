@echo off

start /wait "" ".\bin\Release\net8.0\RpcGenerator.exe"

move /Y "RpcProxy.h" "..\NetworkLibrary\Rpc\"
move /Y "RpcProxy.cpp" "..\NetworkLibrary\Rpc\"
move /Y "RpcStub.h" "..\NetworkLibrary\Rpc\"
move /Y "RpcStub.cpp" "..\NetworkLibrary\Rpc\"

move /Y "DbProxy.h" "..\NetworkLibrary\Database\"
move /Y "DbProxy.cpp" "..\NetworkLibrary\Database\"
move /Y "DbStub.h" "..\NetworkLibrary\Database\"
move /Y "DbStub.cpp" "..\NetworkLibrary\Database\"

del /Q ".\bin\Release\net8.0\RpcProxy.h"
del /Q ".\bin\Release\net8.0\RpcProxy.cpp"
del /Q ".\bin\Release\net8.0\RpcStub.h"
del /Q ".\bin\Release\net8.0\RpcStub.cpp"

del /Q ".\bin\Release\net8.0\DbProxy.h"
del /Q ".\bin\Release\net8.0\DbProxy.cpp"
del /Q ".\bin\Release\net8.0\DbStub.h"
del /Q ".\bin\Release\net8.0\DbStub.cpp"

exit
