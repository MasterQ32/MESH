
using System;
using System.Linq;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Threading;

static class Client
{
	private static byte LocalAddress = 0xFE;

	public static int Main(string[] args)
	{
		var port = new SerialPort("/dev/ttyUSB1", 9600);
		port.Open();
		
		Console.WriteLine("Hello World!");
		
		var receiver = new Thread(() => Receive(port));
		receiver.IsBackground = true;
		receiver.Start();
		
		MainMenu(port);
		
		port.Close();
		
		return 0;
	}
	
	private static void MainMenu(SerialPort port)
	{
		var w = Console.Error;
		while(true)
		{
			w.WriteLine("Main Menu:");
			w.WriteLine("1: Discover (Broadcast)");
			w.WriteLine("2: Ping Device");
			w.WriteLine("3: Device Info");
			w.WriteLine("0: Quit");
			switch(Console.ReadLine().ToInt32())
			{
				case 1:
					SendPacket(port, new Message { 
						Sender = LocalAddress,
						Receiver = 0xFF,
						Command = 0x01,
					});
					break;
				case 2:
				{
					Console.Write("Address:");
					int addr = Console.ReadLine().ToInt32(16);
					SendPacket(port, new Message { 
						Sender = LocalAddress,
						Receiver = (byte)addr,
						Command = 0x01,
					});
					break;
				}
				case 3:
				{
					Console.Write("Address:");
					int addr = Console.ReadLine().ToInt32(16);
					SendPacket(port, new Message { 
						Sender = LocalAddress,
						Receiver = (byte)addr,
						Command = 0x02,
						Data = new byte[] { 0x01 }
					});
					Thread.Sleep(100);
					SendPacket(port, new Message { 
						Sender = LocalAddress,
						Receiver = (byte)addr,
						Command = 0x02,
						Data = new byte[] { 0x02 }
					});
					break;
				}
				case 0:
					return;
				default:
					w.WriteLine("Unknown entry.");
					break;
			}
			Thread.Sleep(100);
		}
	}
	
	private static void SendPacket(SerialPort port, Message msg)
	{
		msg.CalculateChecksum();
		var header = new byte[]
		{
			0x77,
			msg.Flags,
			msg.Receiver,
			msg.Sender,
			msg.Command,
			msg.Length
		};
		port.Write(header, 0, header.Length);
		if(msg.Length > 0) {
			port.Write(msg.Data, 0, msg.Length);
		}
		port.Write(BitConverter.GetBytes(msg.Checksum), 0, 2);
	}
	
	private static Message ReceivePacket(SerialPort port)
	{
		if(port.ReadByte() != 0x77) {
			return null;
		}
		var msg = new Message();
		msg.Flags = (byte)port.ReadByte();
		msg.Receiver = (byte)port.ReadByte();
		msg.Sender = (byte)port.ReadByte();
		msg.Command = (byte)port.ReadByte();
		var length = (byte)port.ReadByte();
		var data = new byte[length];
		for(int i = 0; i < data.Length; i++)
		{
			data[i] = (byte)port.ReadByte();
		}
		msg.Data = data;
		msg.Checksum = (byte)port.ReadByte();
		return msg;
	}
	
	private static void Receive(SerialPort port)
	{
		while(true)
		{
			var msg = ReceivePacket(port);
			if(msg != null)
			{
				Console.WriteLine("Message received:");
				Console.WriteLine("Flags:    {0:X2}", msg.Flags);
				Console.WriteLine("Receiver: {0:X2}", msg.Receiver);
				Console.WriteLine("Sender:   {0:X2}", msg.Sender);
				Console.WriteLine("Command:  {0:X2}", msg.Command);
				Console.WriteLine("Length:   {0:X2}", msg.Length);
				Console.WriteLine("Data:     [{0}]",    msg.Data.Select(b => b.ToString("X2")).Join(" "));
				Console.WriteLine("Checksum: {0:X2}", msg.Checksum);
			}
		}
	}

	private static string Join<T>(this IEnumerable<T> val, string sep) => string.Join(sep, val);
	
	private static int ToInt32(this string val, int b = 10) => Convert.ToInt32(val, b);
}