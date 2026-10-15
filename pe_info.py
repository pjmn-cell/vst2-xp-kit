#!/usr/bin/env python3
"""Résumé d'une DLL (pour COMPARER un plugin qui marche avec le nôtre). Usage : pe_info.py x.dll [y.dll ...]"""
import sys, datetime
import pefile
for p in sys.argv[1:]:
    pe = pefile.PE(p); o = pe.OPTIONAL_HEADER
    ts = datetime.datetime.utcfromtimestamp(pe.FILE_HEADER.TimeDateStamp).strftime("%Y-%m-%d")
    print("=== %s" % p)
    print(" machine=%#x  linker=%d.%d  compilé=%s  caractéristiques=%#x  dll_char=%#x" % (pe.FILE_HEADER.Machine, o.MajorLinkerVersion, o.MinorLinkerVersion, ts, pe.FILE_HEADER.Characteristics, o.DllCharacteristics))
    print(" base=%#x  align fichier=%#x section=%#x  OS %d.%d  sous-système %d.%d (%d)" % (o.ImageBase, o.FileAlignment, o.SectionAlignment, o.MajorOperatingSystemVersion, o.MinorOperatingSystemVersion, o.MajorSubsystemVersion, o.MinorSubsystemVersion, o.Subsystem))
    print(" sections:", " ".join("%s(%#x)" % (s.Name.rstrip(b"\0").decode(), s.Misc_VirtualSize) for s in pe.sections))
    if hasattr(pe, "DIRECTORY_ENTRY_EXPORT"): print(" exports:", [e.name.decode() if e.name else e.ordinal for e in pe.DIRECTORY_ENTRY_EXPORT.symbols])
    for d in getattr(pe, "DIRECTORY_ENTRY_IMPORT", []):
        n = [i.name.decode() for i in d.imports if i.name]
        print(" import %-14s %2d : %s" % (d.dll.decode(), len(n), ", ".join(n)[:150]))
    print(" TLS:", bool(o.DATA_DIRECTORY[9].Size), " ressources:", hasattr(pe, "DIRECTORY_ENTRY_RESOURCE"))
