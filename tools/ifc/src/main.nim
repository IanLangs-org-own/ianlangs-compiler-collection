import os, strutils, sequtils, osproc
import gen_files, compile
import transpiler

proc printHelp() =
  echo """
Uso: ifc archivo1.fcpp [archivo2.fcpp ...] [flags g++]

Opciones:
  -o<nombre>     Especifica el nombre del binario de salida (por defecto: app)
  -cpp<nombre>   Solo genera el archivo .cpp con el nombre indicado, no compila ni borra caches
  -c<nombre>     Genera .cpp y compila a objeto .o, no linkea ni borra otros objetos

Ejemplos:
  ifc main.fcpp -O2 -omiApp        # flujo normal: compila y linkea a dist/miApp
  ifc test.fcpp -cppTest            # solo genera dist/cpp/Test.cpp
  ifc lib.fcpp -cLib                # genera dist/cpp/Lib.cpp y dist/obj/Lib.o
"""

when isMainModule:
  if paramCount() == 0:
    printHelp()
    quit(QuitFailure)
  
  let args = commandLineParams()

  if paramCount() == 1 and args[0] == "-h":
    printHelp()
    quit(0)

  if paramCount() == 1 and args[0] == "-v":
    echo "ifc version 3.0.1\nflow c++ version 3.2"
    quit(0)
    
  # --- Modo solo cpp: -cppX ---
  if paramCount() == 2 and args[1].startsWith("-cpp"):
    let f = args[1]
    if f.len > 4:
      let outName = f[4..^1]  # -cppX → X
      let src = readFile(args[0])
      let cpp = transpile(src)
      writeFile(cacheDir / (outName & ".cpp"), cpp)
      quit(0)

  # --- Modo solo compilar a objeto: -cX ---
  if paramCount() == 2 and args[1].startsWith("-c"):
    let f = args[1]
    if f.len > 2:
      let outName = f[2..^1]  # -cX → X
      let src = readFile(args[0])
      let cpp = transpile(src)
      let cppPath = genCppFile(outName, cpp)
      let obj = objDir / (outName & ".o")
      let cmd = @["g++", "-c", cppPath, "-o", obj]
      let res = execCmd(cmd.join(" "))
      if res != 0:
        quit("Error compilando " & cppPath, QuitFailure)
      quit(0)

  # --- Flujo normal ---
  initDirs()

  # Separar inputs y flags
  let inputs = args.filterIt(it.endsWith(".fcpp"))
  var flags  = args.filterIt(not it.endsWith(".fcpp"))

  if inputs.len == 0:
    quit("No se pasaron archivos .fcpp", QuitFailure)

  # Detectar -o<nombre>
  var outName: string = "app"
  for i, f in flags:
    if f.startsWith("-o"):
      if f.len > 2:
        outName = f[2..^1]
        flags.del(i)
      break

  # Generar los archivos .cpp y recolectar los que se compilarán
  var cppToCompile: seq[string] = @[]
  for file in inputs:
    if not fileExists(file):
      quit("Archivo no existe: " & file, QuitFailure)

    let src = readFile(file)
    let cpp = transpile(src)

    let name = file.extractFilename.changeFileExt("")
    let cppPath = genCppFile(name, cpp)
    cppToCompile.add(cppPath)

  # Compilar y linkear solo esos
  compileAll(flags, cppToCompile, outName)
