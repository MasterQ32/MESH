using System;
using System.Linq;


class Message
{
	public byte Flags { get; set; } = 0x20;
	public byte Sender { get; set; }
	public byte Receiver { get; set; }
	public byte Command { get; set; }
	public byte[] Data { get; set; }
	public byte Length => (byte)(this.Data?.Length ?? 0);
	public byte Checksum { get; set; }
	
	public void CalculateChecksum()
	{
		if(Data == null) {
			this.Checksum = 0;
		} else {
			this.Checksum = (byte)Data.Sum(b => b);
		}
	}
}