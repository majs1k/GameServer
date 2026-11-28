@echo off

start /wait "" ".\bin\Release\net8.0\RpcGenerator.exe"

move /Y "RpcProxy.h" "..\NetworkLibrary\Rpc\"
move /Y "RpcProxy.cpp" "..\NetworkLibrary\Rpc\"
move /Y "RpcStub.h" "..\NetworkLibrary\Rpc\"
move /Y "RpcStub.cpp" "..\NetworkLibrary\Rpc\"
move /Y "RpcDispatcher.h" "..\NetworkLibrary\Rpc\"
move /Y "RpcDispatcher.cpp" "..\NetworkLibrary\Rpc\"

exit
