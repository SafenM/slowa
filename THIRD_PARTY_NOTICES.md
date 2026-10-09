# Third-party notices

## Polish dictionary (`slownik.txt`, `web/slownik.txt.gz`)

The word list used by this project is derived from the **SJP.PL game
dictionary** ("Słownik do gier (literaki, skrable itp.)"), published by the
SJP.PL team.

- **Work:** Słownik do gier (literaki, skrable itp.)
- **Author / copyright holder:** © Zespół SJP.PL
- **Source:** https://sjp.pl/sl/growy/
- **License:** Creative Commons Attribution 4.0 International (CC BY 4.0),
  https://creativecommons.org/licenses/by/4.0/

SJP.PL offers this list under a choice of **GPL 2** or **CC BY 4.0**. This
project uses the **CC BY 4.0** option.

### Modifications

The file in this repository is a **modified subset** of the published list:

- Words that contain characters outside the 32-letter alphabet used by the
  solver (`PARAMS::Alphabet`, i.e. `aąbcćdeęfghijklłmnńoóprsśtuwyzźż`) are
  removed — in particular all words containing `x`, `v` or `q`.
- Two-letter words are removed.
- The remaining words are stored as UTF-8, one word per line.

No other changes are documented for this copy.

### Attribution required by CC BY 4.0

Anyone redistributing this dictionary, or a work based on it, must:

1. credit SJP.PL (© Zespół SJP.PL),
2. link to the CC BY 4.0 license,
3. link to the source, and
4. indicate that changes were made.

The web application shows this attribution in its footer.

---

## This project's own code

All other source code in this repository is licensed under the **MIT License**;
see [`LICENSE`](LICENSE).
