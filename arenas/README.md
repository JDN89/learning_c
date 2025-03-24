## LINKs
[c-cpu-cache-alignment](https://en.ittrip.xyz/c-language/c-cpu-cache-alignment) \
[stackoverflow -- purpose-of-memory-alignment](https://stackoverflow.com/questions/381244/purpose-of-memory-alignment) \
[oud IBM artickle?](https://web.archive.org/web/20201021053824/https://developer.ibm.com/technologies/systems/articles/pa-dalign/) \
[memory-allocation-strategies-002](https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/) 

### Memory alignament
Als je data niet goed uitgelegd is in memory heb je cache missess. Ik begrijp er het fijne niet van vandaar dat ik bovenstaande artikels verzameld heb.

## TODO 
lees de artikels en vat ze hieronder samen
- bekijk ook over de bitwise AND & operator -> ik snap hier het fijne nog niet van.

## Samenvatting
Arena allocators zorgen ervoor dat je data met een gelijke lifetime groepeert in dezelfde Arena.
Wanneer je de data niet meer nodig hebt free je ze in een keer. Dit alles zorgt ervoor dat je makkelijker over data en hun liftimes kan redeneren.
Je moet niet constant Malloc en dan free doen per object, maar gewoon free_arena voor al de objecten in je Arena.

Het probleem dat Bill aanhaaalde in zijn artikel is dat als je niet oppast je cache missess kan tegenkomen. Om dit te voorkomen moet je ervoor zorgen dat je bytes tot de kracth van 2 aligned zijn(1,2,4,8,16,...).

Hiervoor doen we een bitwise operation

``` C 
bool is_power_of_two(uintptr_t x) {
	return (x & (x-1)) == 0;
}
```
1 -> 0001 \
2 -> 0010 \
4 -> 0100 \
8 -> 1000 \
16 -> 10000 \

Wanneer je - 1 doet flip je, voor de binary nummers met de kracht van 2, de bits na de 1 om. 3 -> 0011
Als je dan de & doet kan je dan de LSB vergelijken en zien of iets 0 -> dus kracht van 2 is

##### Voorbeeld 8
1000 -> nummer voor het welk we willen kijken of het kracht van 2 is \
0111 \
---- \
|xxx \
|000 -> power of 2 \

##### Voorbeeld 12 (12-8)
1100 -> 12 \
0111 -> 7 \
|100 -> niet 0 \
remainder is 100 -> 4 \



