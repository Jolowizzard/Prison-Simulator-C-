# Symulacja więzienia

## Założenia projektowe

Tematem symulacji będzie placówka więzienna, składająca się z różnych pomieszczeń. Każdy więzień będzie traktowany jako osobny program, któremu na początku głównego programu zostanie wydzielony odpowiedni obszar w pamięci. Wynika z tego, że więzień będzie osobnym procesem, który podczas pracy symulacji będzie próbował wykonywać odgórnie zaprogramowane akcje. 

Zachowanie więźnia będzie zależne od jego poziomu niezadowolenia i ogólnego zdrowia psychicznego. Przy wyższym poziomie agresji więzień będzie próbował wdać się w bójkę z innym więźniem znajdującym się w tym samym pomieszczeniu, tym samym raniąc go lub samemu zostając rannym. Liczba ran nabytych w wyniku bójki będzie zależna od czasu jej trwania - bójka może zostać natychmiast przerwana, jeżeli strażnik jest dostępny (semafor ze strażnikami). Rany zostają opatrzone i wyleczone w więziennym szpitalu. 

Poszczególne pomieszczenia będą wpływać w różny sposób na poziom agresji i zdrowie psychiczne więźnia, przykładowo:
* **Izolatka** – będzie zmniejszać poziom agresji i wartość zdrowia psychicznego,
* **Stołówka** – będzie zmniejszać poziom głodu (utrzymujący się wysoki poziom głodu będzie stopniowo zwiększać poziom agresji).

Tak samo, w każdym pomieszczeniu może znajdować się różny personel, który będzie zajmował się więźniami. W wyniku symulacji użytkownik otrzyma zbiór danych potrzebnych do ulepszenia swojego planu więzienia - zmiana liczby personelu, dodanie innych pomieszczeń itp. Proces symulacji będzie można śledzić w GUI wyświetlanym w terminalu (biblioteka `ncurses`).

## Realizacja poszczególnych modułów

### 1. Więzienie
Graf symetryczny, w którym korytarze są krawędziami, a pokoje wierzchołkami. Waga krawędzi to koszt podróży z jednego pokoju do drugiego.

### 2. Pokoje
Struktury przechowujące informacje o:
* typie pokoju (w tej wersji projektu są to: *Cela*, *Stołówka*, *Lobby*, *Pralnia*, *Szpital*),
* liczbie więźniów w danym pokoju,
* liczbie czystej i brudnej pościeli.

### 3. Więźniowie
Procesy „żyjące” wewnątrz symulacji. Zaimplementowano prostą maszynę stanów, wedle której determinowane są akcje więźnia. 
* Z czasem trwania symulacji stopień głodu więźnia wzrasta. 
* Od pewnego poziomu głodu zaczyna wzrastać agresja. 
* Gdy agresja osiągnie pewien pułap, więzień zacznie szukać innego więźnia, z którym wda się w bójkę. 
* Gdy w wyniku wygłodzenia albo obrażeń otrzymanych w walce zdrowie więźnia spadnie do poziomu `0`, więzień umiera i przestaje interaktować z systemem.

### 4. Obsługa więzienia
Personel nie liczy się do limitu osób w pokojach, z wyjątkiem Strażnika (zakładamy, że zawsze się jakoś w nich upchnie). Personel składa się z:
* **Strażnika** – patroluje więzienie i rozdziela powstałe bójki, jeżeli są aktywne.
* **Woźnego** – wymienia pościel (inkrementacja semafora pościeli) na świeżą i zawozi zużytą do pralni.
* **Kucharza** – gotuje potrawy w stołówce i zwiększa wartość semafora z informacją o gotowych posiłkach.
* **Lekarza** – odnawia punkty zdrowia więźnia przez określony czas.

### 5. PathFinder
Infrastruktura odpowiedzialna za nawigację. W celu znajdywania najkrótszej ścieżki zaimplementowano **algorytm Dijkstry**.

## Uruchomienie

Program stworzono na systemie linux. Plik wykonywalny jako argument przyjmuje ścieżkę do pliku konfiguracyjnego. W przypadku braku pliku zostanie załadowany domyślny layout więzienia.

### Przykładowy plik konfiguracyjny

\# Liczba pracownikow
GUARDS 2
COOKS 2
JANITORS 1
DOCTORS 1

\# Liczba wiezniow
PRISONERS 6

\# Pokoje: ID TYPE CAPACITY
ROOM 0 CANTINE 10
ROOM 1 CELL 3
ROOM 2 CELL 3
ROOM 3 LOBBY 5
ROOM 4 LAUNDRY 2
ROOM 5 HOSPITAL 5

\# Korytarze: FROM TO DISTANCE
LINK 0 1 3
LINK 0 2 3
LINK 0 3 2
LINK 1 2 2
LINK 4 3 2
LINK 3 5 3
LINK 1 4 5

