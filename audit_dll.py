#!/usr/bin/env python3
"""Audit d'une DLL VST2 32 bits pour Windows XP / Nuendo 2. Usage : audit_dll.py a.dll [b.dll ...]
Vérifie tout ce qui a déjà causé un blocage : fonctions Windows trop récentes, SSE, TLS, symboles de debug,
exports (uniquement 'main'), format du fichier. Code de sortie 1 si un contrôle échoue."""
import sys, subprocess, re
import pefile

ALLOWED_DLLS = {"KERNEL32.DLL", "USER32.DLL", "GDI32.DLL"}
# fonctions qui n'existent pas sous XP (ou pas sans service pack) : leur présence = échec
FORBIDDEN = {"GetThreadId", "InitializeCriticalSectionEx", "GetTickCount64", "InitializeSRWLock", "AcquireSRWLockExclusive",
             "ReleaseSRWLockExclusive", "SleepConditionVariableSRW", "WakeAllConditionVariable", "CreateSymbolicLinkA",
             "GetLogicalProcessorInformationEx", "FlsAlloc", "GetSystemTimePreciseAsFileTime", "InitOnceExecuteOnce"}
OBJDUMP = "i686-w64-mingw32-objdump"

def audit(path):
    errs, warns = [], []
    pe = pefile.PE(path); o = pe.OPTIONAL_HEADER
    if pe.FILE_HEADER.Machine != 0x14c: errs.append("pas du x86 32 bits (Machine=%#x)" % pe.FILE_HEADER.Machine)
    if not (pe.FILE_HEADER.Characteristics & 0x2000): errs.append("pas une DLL")
    exports = [e.name.decode() for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name] if hasattr(pe, "DIRECTORY_ENTRY_EXPORT") else []
    if exports != ["main"]: errs.append("exports = %s (attendu : ['main'] seul, comme les plugins qui marchent)" % exports)
    imports = {}
    for d in getattr(pe, "DIRECTORY_ENTRY_IMPORT", []):
        dll = d.dll.decode().upper(); imports[dll] = [i.name.decode() for i in d.imports if i.name]
        if dll not in ALLOWED_DLLS: errs.append("DLL importée non autorisée : %s (runtime C ? msvcrt/libstdc++ ?)" % dll)
        for f in imports[dll]:
            if f in FORBIDDEN: errs.append("fonction absente de Windows XP : %s!%s" % (dll, f))
    secs = [s.Name.rstrip(b"\0").decode() for s in pe.sections]
    if any(s.startswith(".debug") for s in secs): warns.append("symboles de debug présents (utiliser -s)")
    if ".tls" in secs: errs.append("section .tls présente (TLS dans une DLL chargée dynamiquement : dangereux sous XP)")
    tls = o.DATA_DIRECTORY[9]
    if tls.Size: errs.append("répertoire TLS non vide")
    if o.MajorSubsystemVersion > 5: errs.append("sous-système %d.%d trop récent pour XP" % (o.MajorSubsystemVersion, o.MinorSubsystemVersion))
    if o.ImageBase != 0x10000000: warns.append("ImageBase %#x (les plugins de référence ont 0x10000000)" % o.ImageBase)
    if o.FileAlignment != 0x1000: warns.append("FileAlignment %#x (référence : 0x1000)" % o.FileAlignment)
    try:
        dis = subprocess.run([OBJDUMP, "-d", path], capture_output=True, text=True, check=True).stdout
        sse = len(re.findall(r"%xmm|%mm[0-7]", dis)); cmov = len(re.findall(r"\bcmov", dis))
        if sse: errs.append("%d instructions SSE/MMX (planteront sur un CPU sans SSE2)" % sse)
        if cmov: warns.append("%d instructions cmov (Pentium Pro+ : OK en pratique)" % cmov)
    except Exception as e:
        warns.append("désassemblage impossible (%s) : SSE non vérifié" % e)
    return errs, warns, imports

ok = True
for p in sys.argv[1:]:
    errs, warns, imports = audit(p)
    status = "OK " if not errs else "ÉCHEC"
    print("[%s] %s%s" % (status, p.split("/")[-1], "   (" + ", ".join(sorted(imports)) + ")" if not errs else ""))
    for e in errs: print("      ERREUR  :", e)
    for w in warns: print("      attention:", w)
    ok &= not errs
sys.exit(0 if ok else 1)
