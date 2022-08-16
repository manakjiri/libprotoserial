
# prealokace
požadavky od každé vrstvy

- interface potřebuje header a footer
- potenciální security potřebuje hlavičku i footer
- fragmentation nepotřebuje prealokaci
- porty potřebují hlavičku 

chce to něco ve smyslu data_size objektu, který nese doporučené velikosti front a back. Každá vrstva definuje constantu s tímto objektem, objekt podporuje sčítání a předává se vyšším vrstvám
