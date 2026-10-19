# X Cleaner — Desktop & CLI

**English** · [한국어 / detailed original guide](README.ko.md)

In a suitable Python environment, install `requirements.txt` and run `python -m x_cleaner gui` from this directory. Development dependencies are in `requirements-dev.txt`.

An offline demonstration can be prepared with:

```sh
python -m x_cleaner preview --archive examples/archive --selection-file examples/selection.example.csv --job jobs/demo-preview
python -m x_cleaner run --job jobs/demo-preview
```

The example archive is synthetic. Job outputs are generated locally and are not included in this repository. These commands are previews; do not use the example data for real-account operations.

Live execution requires the user's own archive, an explicit selection, authentication, account/target verification, and a separate confirmation. The Korean guide retains the detailed procedures, recovery behavior, and limitations. Historical API limits or prices in that guide must be checked against current official documentation before live use.

[Portfolio overview](../../../README.md) · [Attribution](../../../THIRD_PARTY_NOTICES.md)
