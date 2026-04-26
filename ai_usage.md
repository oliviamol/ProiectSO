# AI Usage - Phase 1

## Ce tool am folosit
Claude

## Pentru ce am folosit AI-ul

Am folosit AI doar pentru cele 2 functii cerute in cerinta: `parse_condition` si `match_condition`.

---

## Cum am cerut

### Prompt 1 - parse_condition

I-am dat structura mea Report si i-am explicat ca am nevoie de o functie care primeste un string de forma `field:operator:value` si il imparte in 3 parti separate. I-am zis sa returneze 1 daca a mers si 0 daca nu.

Promptul meu (aproximativ):
 "Am o structura Report in C cu campurile: ID (int), inspectorname (char[31]), latitude si longitude (float), category (char[12]), severitylevel (int), timestamp (time_t), description (char[101]). Am nevoie de o functie int parse_condition(const char *input, char *field, char *op, char *value) care sa separe un string de forma field:operator:value in cele 3 parti. Returneaza 1 daca a functionat, 0 altfel."

### Ce a generat

A generat o varianta cu `sscanf` si `%[^:]`. Era ok dar am preferat sa o rescriu cu parcurgere manuala caracter cu caracter cu un pointer `*p`, ca sa inteleg mai bine ce face si sa pot explica la prezentare.

---

### Prompt 2 - match_condition

Dupa ce i-am dat campurile si tipurile lor, i-am cerut o functie care compara un raport cu o conditie.

Promptul meu (aproximativ):
 "Aceeasi structura Report. Am nevoie de o functie int match_condition(Report *r, const char *field, const char *op, const char *value) care verifica daca raportul satisface conditia. Campuri suportate: severity (int), category (string), inspector (string), timestamp (time_t). Operatori: ==, !=, <, <=, >, >=."

### Ce a generat

A generat functia cu if-uri pentru fiecare camp. Problemele pe care le-am gasit:
- Folosea `atoi()` si pentru timestamp, ceea ce e gresit pe sisteme pe 64 biti unde time_t e long. Am schimbat la `atol()`.
- Pentru campuri necunoscute returna 0, ceea ce inseamna ca ar fi exclus rapoarte fara motiv. Am schimbat la `return 1` (daca nu stim campul, nu filtram dupa el).
- Nu avea toate operatorii pentru toate campurile (de ex lipsea `<` la category, dar asta e corect oricum pentru string-uri).

---

## Ce am schimbat

1. `parse_condition` - am rescris-o manual cu pointer in loc de sscanf, mai usor de explicat
2. `match_condition` - am schimbat `atoi` cu `atol` pentru timestamp si `return 0` cu `return 1` pentru camp necunoscut

## Ce am invatat

- Ca AI-ul genereaza cod care pare corect dar are greseli subtile (atoi vs atol)
- Ca trebuie sa testezi fiecare caz in parte, nu sa presupui ca merge
- Ca e mai bine sa intelegi codul generat si sa il rescrii cu cuvintele tale decat sa il copiezi direct
