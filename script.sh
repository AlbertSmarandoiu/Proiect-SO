#!/bin/bash

# verificam daca exista un argument
if [ "$#" -ne 1 ]; then 
    echo "Numărul de argumente este total incorect!"
    exit 1
fi 

nume_fisier="$1"

# verificam existenta si accesibilitatea fisierului
if [ ! -f "$nume_fisier" ]; then
    echo "Fișierul $nume_fisier nu există sau nu este un fișier!"
    exit 1
fi

# verificam faca utilizatorul are permisiunea de citire a fisierului
if [ ! -r "$nume_fisier" ]; then
    echo "Nu ai permisiunea de a citi fișierul $nume_fisier"
    exit 1
fi

# salvam drepturile initiale ale fisierului
drepturi_initiale=$(stat -c %a "$nume_fisier")

# acordam drepturile 777 daca fisierul nu are nici un drept
if [ "$drepturi_initiale" == "000" ]; then
    echo "Fișierul nu are niciun drept. Acordăm drepturile 777."
    chmod 777 "$nume_fisier"
fi

gasit=0

# Verificăm continutul pentru caractele non ascii
if grep -q -P '[^\x00-\x7F]' "$nume_fisier"; then    
    echo "Fișierul $nume_fisier conține caractere non-ASCII!"
    gasit=1
fi 

# Verificăm continutul fisierul pentru cuvintele periculoase
if grep -q -E 'malefic|periculos|parola|attack|rau|mama' "$nume_fisier"; then
    echo "Fișierul $nume_fisier conține cuvinte periculoase"
    gasit=1
fi

# refacem dreptuile initiale ale fisierului
if [ "$drepturi_initiale" == "000" ]; then
    echo "Restaurăm drepturile inițiale."
    chmod "$drepturi_initiale" "$nume_fisier"
fi

exit $gasit