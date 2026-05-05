# Problem czytelników i pisarzy

Projekt przedstawia trzy warianty rozwiązania problemu czytelników i pisarzy:

1. Pierwszeństwo czytelników – możliwe zagłodzenie pisarzy.
2. Pierwszeństwo pisarzy – możliwe zagłodzenie czytelników.
3. Rozwiązanie sprawiedliwe – ograniczenie zagłodzenia obu grup.

## Kompilacja lokalna

```bash
make
## Uruchomienie
./czytelnicy_pierwsi
./pisarze_pierwsi
./sprawiedliwe
## Docker - Budowanie obrazu
docker build -t czytelnicy-pisarze .
## Uruchomienie wybranego wariantu
docker run --rm czytelnicy-pisarze ./czytelnicy_pierwsi
docker run --rm czytelnicy-pisarze ./pisarze_pierwsi
docker run --rm czytelnicy-pisarze ./sprawiedliwe

