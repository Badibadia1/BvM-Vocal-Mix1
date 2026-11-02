#!/usr/bin/env python3
"""Génère un secret aléatoire et l'écrit AU MÊME ENDROIT dans Source/License.h et tools/keygen.py.
Usage : python3 tools/set_secret.py        (à lancer UNE SEULE FOIS, avant la première compilation de vente)
Attention : changer le secret invalide tous les codes déjà vendus. Gardez keygen.py et ce secret PRIVÉS.
"""
import pathlib, re, secrets

root = pathlib.Path(__file__).resolve().parent.parent
lic, key = root / "Source" / "License.h", root / "tools" / "keygen.py"
secret = secrets.token_urlsafe(36)

h = lic.read_text(encoding="utf-8")
h2, n1 = re.subn(r'(kSecret\s*=\s*)"[^"]*"', lambda m: f'{m.group(1)}"{secret}"', h)
k = key.read_text(encoding="utf-8")
k2, n2 = re.subn(r'(SECRET\s*=\s*)"[^"]*"', lambda m: f'{m.group(1)}"{secret}"', k)
if n1 != 1 or n2 != 1:
    raise SystemExit("Secret introuvable dans License.h ou keygen.py : rien n'a été modifié.")
lic.write_text(h2, encoding="utf-8"); key.write_text(k2, encoding="utf-8")
print("Nouveau secret écrit dans License.h et keygen.py. Recompilez le plugin.")
