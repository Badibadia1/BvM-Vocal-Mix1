#!/usr/bin/env python3
"""Keygen BvM : génère le code d'activation à partir de l'ID machine.

Usage :
    keygen <ID_MACHINE>
    keygen                 (le programme demande l'ID)

Le secret doit être IDENTIQUE à kSecret dans Source/License.h.
Ordre de recherche du secret :
    1. variable d'environnement BVM_SECRET
    2. fichier secret.txt placé à côté du programme
    3. valeur par défaut ci-dessous
"""
import hashlib
import os
import sys

DEFAULT_SECRET = "CHANGE-MOI-SECRET-LONG-ET-ALEATOIRE-2026"


def load_secret() -> str:
    env = os.environ.get("BVM_SECRET")
    if env:
        return env
    base = os.path.dirname(sys.executable if getattr(sys, "frozen", False)
                           else os.path.abspath(__file__))
    path = os.path.join(base, "secret.txt")
    if os.path.isfile(path):
