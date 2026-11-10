#!/usr/bin/env python3
import sqlite3
import os
import re
import argparse

def convert_int_to_hex(src_db):
    if not os.path.exists(src_db):
        raise FileNotFoundError(src_db)

    dst_db = os.path.splitext(src_db)[0] + "_hex.db"

    # delete existing dest db
    if os.path.exists(dst_db):
        os.remove(dst_db)

    # load source db into memory
    src = sqlite3.connect(src_db)
    src.row_factory = sqlite3.Row
    mem = sqlite3.connect(":memory:")
    mem.row_factory = sqlite3.Row
    src.backup(mem)

    dst = sqlite3.connect(dst_db)
    cur = mem.cursor()

    # enumerate tables
    cur.execute("SELECT name FROM sqlite_master WHERE type='table' AND name!='sqlite_sequence'")
    tables = [r[0] for r in cur.fetchall()]

    for table in tables:
        # get column info
        cur.execute(f"PRAGMA table_info({table})")
        cols = cur.fetchall()

        names = [c[1] for c in cols]
        int_cols = {c[1] for c in cols if c[2].upper() == "INT" and c[1] not in ("ID", "STEP")}

        # get original CREATE TABLE
        cur.execute("SELECT sql FROM sqlite_master WHERE type='table' AND name=?", (table,))
        create_sql = cur.fetchone()[0]

        # rewrite INT -> TEXT for selected columns
        for c in int_cols:
            create_sql = re.sub(
                rf"\b{c}\b\s+INT\b([^,)]*)",
                rf"{c} TEXT\1",
                create_sql,
                flags=re.IGNORECASE
            )

        dst.execute(create_sql)

        # copy data
        cur.execute(f"SELECT * FROM {table}")
        for row in cur.fetchall():
            out = []
            for k in names:
                v = row[k]
                if k in int_cols and v is not None:
                    out.append(hex(v & 0xFFFFFFFFFFFFFFFF))
                else:
                    out.append(v)

            dst.execute(
                f"INSERT INTO {table} VALUES ({','.join(['?'] * len(out))})",
                out
            )

    dst.commit()
    src.close()
    mem.close()
    dst.close()

    print(f"Converted DB saved to: {dst_db}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert INT columns (except ID, STEP) to hex TEXT in a copied SQLite DB"
    )
    parser.add_argument("db", help="Path to source SQLite DB")

    args = parser.parse_args()
    convert_int_to_hex(args.db)
