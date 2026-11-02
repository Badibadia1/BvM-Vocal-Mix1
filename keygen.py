#!/usr/bin/env python3
"""Générateur de codes d'activation BvM (à GARDER PRIVÉ, ne jamais distribuer).
Usage : python3 keygen.py XXXX-XXXX-XXXX-XXXX     (ID machine envoyé par le client)
Le SECRET doit être identique à License::kSecret dans Source/License.h.
"""
import hashlib, sys

SECRET = "CHANGE-MOI-SECRET-LONG-ET-ALEATOIRE-2026"

if len(sys.argv) != 2:
    sys.exit("Usage : keygen.py <ID-MACHINE>")
mid = sys.argv[1].strip().upper()
h = hashlib.sha256(f"{SECRET}:{mid}".encode("utf-8")).hexdigest().upper()[:20]
print("-".join(h[i:i + 5] for i in range(0, 20, 5)))
