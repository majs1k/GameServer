using PacketGenerator;

class RpcGenerator
{
    public static void Generate(string readFile)
    {
        GenerateRpcProxy(readFile);
        GenerateRpcStub(readFile);
    }

    // RpcProxy
    public static void GenerateRpcProxy(string readFile)
    {
        using (var reader = new StreamReader(readFile))
        {
            string proxyHeaderFunc = "";
            string proxyCppFunc = "";
            string proxyHeaderFuncVector = "";
            string proxyCppFuncVector = "";

            string line;

            while ((line = reader.ReadLine()) != null)
            {
                if (string.IsNullOrWhiteSpace(line))
                    continue;

                var parsed = Parser.ParseLine(line);

                string funcParam = "";
                string shiftParam = "";

                foreach (var p in parsed.Parameters)
                {
                    funcParam += ", ";
                    funcParam += string.Format(
                        RpcFormat.funcParam,
                        p.type,
                        p.name);

                    shiftParam += string.Format(
                        RpcFormat.shiftLeft,
                        p.name);
                }

                // 기존 단일 세션 함수
                proxyHeaderFunc += string.Format(
                    RpcFormat.proxyHeaderFunc,
                    parsed.Name,
                    funcParam);

                proxyCppFunc += string.Format(
                    RpcFormat.proxyCppFunc,
                    parsed.Name,
                    funcParam,
                    parsed.PacketType,
                    shiftParam);

                // 벡터 세션 함수
                proxyHeaderFuncVector += string.Format(
                    RpcFormat.proxyHeaderFuncVector,
                    parsed.Name,
                    funcParam);

                proxyCppFuncVector += string.Format(
                    RpcFormat.proxyCppFuncVector,
                    parsed.Name,
                    funcParam,
                    parsed.PacketType,
                    shiftParam);
            }

            // 두 함수 선언을 함께 생성
            string proxyHeader = string.Format(
                RpcFormat.proxyHeader,
                proxyHeaderFunc + proxyHeaderFuncVector);

            File.WriteAllText("RpcProxy.h", proxyHeader);

            // 두 함수 구현을 함께 생성
            string proxyCpp =
                RpcFormat.proxyCppHeader +
                Environment.NewLine +
                proxyCppFunc +
                Environment.NewLine +
                proxyCppFuncVector;

            File.WriteAllText("RpcProxy.cpp", proxyCpp);
        }
    }

    // RpcStub
    public static void GenerateRpcStub(string readFile)
    {
        using (var reader = new StreamReader(readFile))
        {
            string stubHeaderFunc = "";
            string stubPacketProc = "";
            string stubHeaderMessageCount = "";
            string stubCppFunc = "";
            string line;

            while ((line = reader.ReadLine()) != null)
            {
                if (string.IsNullOrWhiteSpace(line))
                    continue;

                var parsed = Parser.ParseLine(line);
                string funcParam = "";
                string funcParam1 = "";
                string funcParam2 = "";
                string shiftParam = "";

                stubHeaderMessageCount += string.Format(
                  RpcFormat.stubHeaderMessageCount,
                     parsed.Name);

                foreach (var p in parsed.Parameters)
                {
                    funcParam += ", ";
                    funcParam += string.Format(RpcFormat.funcParam, p.type, p.name);

                    funcParam1 += ", ";
                    funcParam1 += p.name;

                    string localType = p.type.Replace("&", "").Trim();

                    funcParam2 += "\t\t";
                    funcParam2 += string.Format(RpcFormat.funcParam, localType, p.name);
                    funcParam2 += ";";
                    funcParam2 += Environment.NewLine;

                    shiftParam += string.Format(RpcFormat.shiftRight, p.name);
                }

                stubHeaderFunc += string.Format(
                    RpcFormat.stubHeaderFunc,
                    parsed.Name,
                    funcParam);

                stubPacketProc += string.Format(
                    RpcFormat.stubPacketProcCase,
                    parsed.PacketType,
                    funcParam2,
                    shiftParam,
                    parsed.Name,
                    parsed.Name,
                    funcParam1);

                stubCppFunc += string.Format(
                    RpcFormat.stubCppFunc,
                    parsed.Name,
                    funcParam);
            }

            string stubHeader = string.Format(RpcFormat.stubHeader, stubHeaderFunc + Environment.NewLine + stubHeaderMessageCount);

            File.WriteAllText("RpcStub.h", stubHeader);

            string stubCpp =
                RpcFormat.stubCppHeader +
                Environment.NewLine +
                string.Format(RpcFormat.stubCppPacketProc, stubPacketProc) +
                Environment.NewLine +
                stubCppFunc;

            File.WriteAllText("RpcStub.cpp", stubCpp);
        }
    }
}
