
using PacketGenerator;

class RpcGenerator
{
    public static void Generate(string readFile)
    {
        GenerateRpcProxy(readFile);
        GenerateRpcStub(readFile);
        GenerateRpcDispatcher(readFile);
    }

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

            string proxyHeader = string.Format(
                RpcFormat.proxyHeader,
                proxyHeaderFunc + proxyHeaderFuncVector);

            File.WriteAllText("RpcProxy.h", proxyHeader);

            string proxyCpp =
                RpcFormat.proxyCppHeader +
                Environment.NewLine +
                proxyCppFunc +
                Environment.NewLine +
                proxyCppFuncVector;

            File.WriteAllText("RpcProxy.cpp", proxyCpp);
        }
    }

    public static void GenerateRpcStub(string readFile)
    {
        using (var reader = new StreamReader(readFile))
        {
            string stubHeaderFunc = "";
            string stubHeaderMessageCount = "";
            string stubCppFunc = "";
            string line;

            while ((line = reader.ReadLine()) != null)
            {
                if (string.IsNullOrWhiteSpace(line))
                    continue;

                var parsed = Parser.ParseLine(line);

                string funcParam = "";

                foreach (var p in parsed.Parameters)
                {
                    funcParam += ", ";
                    funcParam += string.Format(
                        RpcFormat.funcParam,
                        p.type,
                        p.name);
                }

                stubHeaderFunc += string.Format(
                    RpcFormat.stubHeaderFunc,
                    parsed.Name,
                    funcParam);

                stubHeaderMessageCount += string.Format(
                    RpcFormat.stubHeaderMessageCount,
                    parsed.Name);

                stubCppFunc += string.Format(
                    RpcFormat.stubCppFunc,
                    parsed.Name,
                    funcParam);
            }

            string stubHeader = string.Format(
                RpcFormat.stubHeader,
                stubHeaderFunc,
                stubHeaderMessageCount);

            File.WriteAllText("RpcStub.h", stubHeader);

            string stubCpp =
                RpcFormat.stubCppHeader +
                Environment.NewLine +
                stubCppFunc;

            File.WriteAllText("RpcStub.cpp", stubCpp);
        }
    }

    public static void GenerateRpcDispatcher(string readFile)
    {
        using (var reader = new StreamReader(readFile))
        {
            string dispatcherPacketProc = "";
            string line;

            while ((line = reader.ReadLine()) != null)
            {
                if (string.IsNullOrWhiteSpace(line))
                    continue;

                var parsed = Parser.ParseLine(line);

                string funcParam1 = "";
                string funcParam2 = "";
                string shiftParam = "";

                foreach (var p in parsed.Parameters)
                {
                    funcParam1 += ", ";
                    funcParam1 += p.name;

                    string localType = p.type.Replace("&", "").Trim();

                    funcParam2 += "\t\t";
                    funcParam2 += string.Format(
                        RpcFormat.funcParam,
                        localType,
                        p.name);
                    funcParam2 += ";";
                    funcParam2 += Environment.NewLine;

                    shiftParam += string.Format(
                        RpcFormat.shiftRight,
                        p.name);
                }

                dispatcherPacketProc += string.Format(
                    RpcFormat.dispatcherDispatchPacketCase,
                    parsed.PacketType,
                    funcParam2,
                    shiftParam,
                    parsed.Name,
                    parsed.Name,
                    funcParam1);
            }

            string dispatcherHeader = string.Format(RpcFormat.dispatcherHeader, "");

            File.WriteAllText(
                "RpcDispatcher.h",
                dispatcherHeader);

            string dispatcherCpp =
                RpcFormat.dispatcherCppHeader +
                Environment.NewLine +
                string.Format(
                    RpcFormat.dispatcherCppDispatchPacket,
                    dispatcherPacketProc);

            File.WriteAllText(
                "RpcDispatcher.cpp",
                dispatcherCpp);
        }
    }
}
