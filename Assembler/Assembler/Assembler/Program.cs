using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Assembler
{
	class Program
	{
		const int WordSize = 4;

		enum Opcode
		{
			Nop = 0,
			And = 1,
			Or = 2,
			Not = 3,
			Dup = 4,
			Cmp = 5,
			Inc = 6,
			Pop = 7,
			Ldi1 = 8,
			Ret = 0x3e,
			Halt = 0x3f,
			Load = 0x80,
			Loadi = 0x81,
			Jump = 0x82,
			Call = 0x83,
			Jumpz = 0x84,
			Jumpnz = 0x85,
			Store = 0x86
		}

		abstract class Argument
		{

		}

		class ArgumentLabel : Argument
		{
			public string LabelName { get; set; }
		}

		class ArgumentAddress : Argument
		{
			public uint Address { get; set; }
		}

		abstract class Statement {
			public abstract uint Size { get; }
		}

		class Instruction : Statement
		{
			public Opcode Opcode { get; set; }
			public Argument Argument { get; set; }

			public override uint Size {
				get
				{
					byte opcodeLength = (byte)(((byte)Opcode) >> 6);

					if (opcodeLength == 2)
					{
						return 1 + WordSize;
					} else if (opcodeLength == 1)
					{
						return 2;
					} else
					{
						return 1;
					}
				}
			}
		}

		class DefineBytes : Statement
		{
			public byte[] Bytes { get; set; }

			public override uint Size => (uint)Bytes.Length;
		}

		static void Main(string[] args)
		{
			new Program().RunAsync(args[0], args[1]).Wait();
		}

		private Argument ParseArgument(string argument)
		{
			if (argument.StartsWith("0x"))
			{
				return new ArgumentAddress { Address = uint.Parse(argument.Substring(2), System.Globalization.NumberStyles.HexNumber) };
			}

			if (uint.TryParse(argument, out var number))
			{
				return new ArgumentAddress { Address = number };
			}

			return new ArgumentLabel { LabelName = argument };
		}

		private async Task RunAsync(string assemblyFile, string binaryFile)
		{
			var lines = File.ReadAllLines(assemblyFile);

			var statements = new List<Statement>();
			var labels = new Dictionary<string, uint>();

			uint address = 0;

			foreach (var line in lines)
			{
				var pieces = line.Split(' ');
				var i = 0;
				
				if (pieces[0].EndsWith(":"))
				{
					var labelName = pieces[0].Substring(0, pieces[0].Length - 1);

					labels[labelName] = address;

					i++;
				}

				var opcode = pieces[i].ToLower();

				switch (opcode)
				{
					case "nop":
						{
							statements.Add(new Instruction { Opcode = Opcode.Nop });
						}
						break;
					case "and":
						{
							statements.Add(new Instruction { Opcode = Opcode.And });
						}
						break;
					case "or":
						{
							statements.Add(new Instruction { Opcode = Opcode.Or });
						}
						break;
					case "not":
						{
							statements.Add(new Instruction { Opcode = Opcode.Not });
						}
						break;
					case "dup":
						{
							statements.Add(new Instruction { Opcode = Opcode.Dup });
						}
						break;
					case "cmp":
						{
							statements.Add(new Instruction { Opcode = Opcode.Cmp });
						}
						break;
					case "inc":
						{
							statements.Add(new Instruction { Opcode = Opcode.Inc });
						}
						break;
					case "pop":
						{
							statements.Add(new Instruction { Opcode = Opcode.Pop });
						}
						break;
					case "ldi.1":
						{
							statements.Add(new Instruction { Opcode = Opcode.Ldi1 });
						}
						break;
					case "ret":
						{
							statements.Add(new Instruction { Opcode = Opcode.Ret });
						}
						break;
					case "halt":
						{
							statements.Add(new Instruction { Opcode = Opcode.Halt });
						}
						break;
					case "load":
						{
							statements.Add(new Instruction { Opcode = Opcode.Load, Argument = ParseArgument(pieces[i+1]) });
						}
						break;
					case "loadi":
						{
							statements.Add(new Instruction { Opcode = Opcode.Loadi, Argument = ParseArgument(pieces[i + 1]) });
						}
						break;
					case "jump":
						{
							statements.Add(new Instruction { Opcode = Opcode.Jump, Argument = ParseArgument(pieces[i + 1]) });
						}
						break;
					case "call":
						{
							statements.Add(new Instruction { Opcode = Opcode.Call, Argument = ParseArgument(pieces[i + 1]) });
						}
						break;
					case "jumpz":
						{
							statements.Add(new Instruction { Opcode = Opcode.Jumpz, Argument = ParseArgument(pieces[i + 1]) });
						}
						break;
					case "jumpnz":
						{
							statements.Add(new Instruction { Opcode = Opcode.Jumpnz, Argument = ParseArgument(pieces[i + 1]) });
						}
						break;
					case "store":
						{
							statements.Add(new Instruction { Opcode = Opcode.Store, Argument = ParseArgument(pieces[i + 1]) });
						}
						break;
					case ".db":
						{
							var str = string.Join(" ", Enumerable.Range(i + 1, pieces.Length - i - 1).Select(x=>pieces[x]));
							statements.Add(new DefineBytes { Bytes = Encoding.ASCII.GetBytes(str) });
						}
						break;
					default:
						throw new Exception("Unknown instructions");
				}

				address += statements.Last().Size;
			}

			foreach (var statement in statements)
			{
				if (statement is Instruction)
				{
					var instruction = (Instruction)statement;

					if (instruction.Argument is ArgumentLabel)
					{
						var label = (ArgumentLabel)instruction.Argument;

						var resolved = labels[label.LabelName];

						instruction.Argument = new ArgumentAddress { Address = resolved };
					}
				}
			}

			using (var ms = new MemoryStream())
			{
				var binaryWriter = new BinaryWriter(ms);
				foreach (var statement in statements)
				{
					switch (statement)
					{
						case Instruction instruction:
							{
								binaryWriter.Write((byte)instruction.Opcode);
								var a = instruction.Argument as ArgumentAddress;

								if (a != null)
								{
									binaryWriter.Write(a.Address);
								}
							}
							break;
						case DefineBytes defineBytes:
							{
								binaryWriter.Write(defineBytes.Bytes);
								binaryWriter.Write((byte)0);
							}
							break;
					}
				}

				binaryWriter.Flush();
				ms.Position = 0;
				File.WriteAllBytes(binaryFile, ms.ToArray());
			}
		}
	}
}
