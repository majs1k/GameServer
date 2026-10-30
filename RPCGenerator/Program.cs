namespace PacketGenerator
{
    internal class Program
    {
        static void Main(string[] args)
        {
            var rpcFile = "ChatRpcIdl.txt";
            RpcGenerator.Generate(rpcFile);
            
            //var dbFile = "game_db_idl.txt";
            //DbGenerator.Generate(dbFile);
        }
    }
}
