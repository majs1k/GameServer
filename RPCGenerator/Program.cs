namespace PacketGenerator
{
    internal class Program
    {
        static void Main(string[] args)
        {
            var rpcFile = "ChatRpcIdl.txt";
            RpcGenerator.Generate(rpcFile);
        }
    }
}
