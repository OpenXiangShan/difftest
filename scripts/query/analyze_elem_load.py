#!/usr/bin/env python3

import argparse
import math
import re
import sqlite3
import sys
from pathlib import Path

try:
    import matplotlib.pyplot as plt
    import pandas as pd
    import seaborn as sns
    from matplotlib.ticker import MaxNLocator
except ModuleNotFoundError as exc:
    raise SystemExit(
        "Missing Python package: "
        f"{exc.name}. Use the repo venv: .venv-elem-load/bin/python "
        "difftest/scripts/query/analyze_elem_load.py ..."
    )


DEFAULT_DELTA_LIMIT = 8
DEFAULT_ZOOM_MIN_POINTS = 40
DEFAULT_ZOOM_MAX_POINTS = 120


def parse_int(value):
    if value is None:
        return None
    if isinstance(value, int):
        return value
    text = str(value).strip()
    if not text:
        return None
    return int(text, 16) if text.lower().startswith("0x") else int(text)


def format_coreid_variants(coreid):
    numeric = parse_int(coreid)
    return sorted({str(numeric), hex(numeric), hex(numeric).upper()})


def camel_to_snake(text):
    text = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", text)
    text = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1_\2", text)
    return text.lower()


def simplify_table_name(table_name):
    alias = table_name
    for suffix in ("RegStateElem", "StateElem", "Elem"):
        if alias.endswith(suffix):
            alias = alias[: -len(suffix)]
            break
    return camel_to_snake(alias)


def list_elem_tables(conn):
    query = """
        SELECT name
        FROM sqlite_master
        WHERE type='table' AND name LIKE '%Elem'
        ORDER BY name
    """
    return [row[0] for row in conn.execute(query).fetchall()]


def load_trap_steps(conn, coreid_variants):
    placeholders = ",".join("?" for _ in coreid_variants)
    query = f"""
        SELECT STEP, CYCLECNT
        FROM TrapEvent
        WHERE COREID IN ({placeholders})
        ORDER BY STEP, ID
    """
    rows = conn.execute(query, coreid_variants).fetchall()
    step_to_cycle = {}
    for step, raw_cycle in rows:
        cycle = parse_int(raw_cycle)
        if step in step_to_cycle and step_to_cycle[step] != cycle:
            raise ValueError(f"STEP {step} maps to multiple cycleCnt values")
        step_to_cycle[step] = cycle
    if not step_to_cycle:
        raise ValueError("No TrapEvent rows matched the requested coreid")
    return pd.DataFrame(
        [{"step": step, "cycle": cycle} for step, cycle in sorted(step_to_cycle.items())]
    )


def load_elem_counts(conn, table_name, coreid_variants):
    columns = {row[1] for row in conn.execute(f"PRAGMA table_info({table_name})").fetchall()}
    if "COREID" in columns:
        placeholders = ",".join("?" for _ in coreid_variants)
        query = f"""
            SELECT STEP, COUNT(*) AS total_items
            FROM {table_name}
            WHERE COREID IN ({placeholders})
            GROUP BY STEP
            ORDER BY STEP
        """
        rows = conn.execute(query, coreid_variants).fetchall()
    else:
        query = f"""
            SELECT STEP, COUNT(*) AS total_items
            FROM {table_name}
            GROUP BY STEP
            ORDER BY STEP
        """
        rows = conn.execute(query).fetchall()
    return pd.DataFrame(rows, columns=["step", "total_items"])


def build_cycle_dataframe(trap_df, elem_df, delta_limit):
    df = trap_df.merge(elem_df, on="step", how="left")
    df["total_items"] = df["total_items"].fillna(0).astype(int)
    df["splitter_cycles"] = df["total_items"].map(lambda count: math.ceil(count / delta_limit) if count else 0)
    cycle_df = (
        df.groupby("cycle", as_index=False)[["total_items", "splitter_cycles"]]
        .sum()
        .sort_values("cycle")
    )
    return cycle_df


def select_zoom_window(cycle_df, min_points=DEFAULT_ZOOM_MIN_POINTS, max_points=DEFAULT_ZOOM_MAX_POINTS, zoom_points=None):
    count = len(cycle_df)
    if count <= 1:
        return cycle_df.copy()

    if zoom_points is not None:
        window = max(1, min(int(zoom_points), count))
    else:
        window = min(max(min_points, count // 25), max_points, count)
    if window == count:
        return cycle_df.copy()

    values = cycle_df["total_items"].to_numpy()
    best_start = 0
    best_score = None
    for start in range(0, count - window + 1):
        segment = values[start : start + window]
        seg_max = int(segment.max())
        seg_min = int(segment.min())
        seg_mean = float(segment.mean())
        threshold = max(seg_mean * 1.8, seg_min + (seg_max - seg_min) * 0.7)
        spike_count = int((segment >= threshold).sum()) if seg_max > 0 else 0
        score = (
            int(seg_max - seg_min),
            min(spike_count, 6),
            seg_max,
            int(segment.sum()),
        )
        if best_score is None or score > best_score:
            best_start = start
            best_score = score
    return cycle_df.iloc[best_start : best_start + window].copy()


def configure_plot_style():
    sns.set_theme(style="whitegrid", context="talk")
    plt.rcParams.update(
        {
            "figure.facecolor": "#f8f6f1",
            "axes.facecolor": "#fffdf8",
            "axes.edgecolor": "#5b5b5b",
            "grid.color": "#d8d2c6",
            "grid.alpha": 0.6,
            "axes.titleweight": "bold",
            "axes.labelweight": "medium",
            "savefig.facecolor": "#f8f6f1",
        }
    )


def finalize_axes(ax):
    ax.xaxis.set_major_locator(MaxNLocator(nbins=8, integer=True))
    ax.yaxis.set_major_locator(MaxNLocator(nbins=7, integer=True))
    ax.margins(x=0.01)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)


def nonzero_points(cycle_df, column):
    return cycle_df[cycle_df[column] > 0].copy()

def pulse_linewidth(point_count):
    if point_count <= 4:
        return 3.8
    if point_count <= 16:
        return 3.2
    if point_count <= 64:
        return 2.6
    if point_count <= 256:
        return 2.0
    if point_count <= 1024:
        return 1.4
    return 1.0


def draw_pulses(ax, df, y_col, color, linewidth=None, alpha=0.95):
    if df.empty:
        return
    if linewidth is None:
        linewidth = pulse_linewidth(len(df))
    ax.vlines(df["cycle"], 0, df[y_col], color=color, linewidth=linewidth, alpha=alpha)


def plot_items(cycle_df, alias, output_path):
    fig, ax = plt.subplots(figsize=(15, 5.5))
    draw_pulses(ax, nonzero_points(cycle_df, "total_items"), "total_items", color="#d1495b")
    ax.set_title(f"{alias} items pulse view")
    ax.set_xlabel("cycle")
    ax.set_ylabel("items")
    ax.axhline(0, color="#8d8d8d", linewidth=0.8, alpha=0.8)
    finalize_axes(ax)
    fig.tight_layout()
    fig.savefig(output_path, dpi=220)
    plt.close(fig)


def plot_cycles(cycle_df, alias, output_path):
    fig, ax = plt.subplots(figsize=(15, 5.5))
    draw_pulses(ax, nonzero_points(cycle_df, "splitter_cycles"), "splitter_cycles", color="#2b59c3")
    ax.set_title(f"{alias} DeltaSplitter cycles pulse view")
    ax.set_xlabel("cycle")
    ax.set_ylabel("cycles")
    ax.axhline(0, color="#8d8d8d", linewidth=0.8, alpha=0.8)
    finalize_axes(ax)
    fig.tight_layout()
    fig.savefig(output_path, dpi=220)
    plt.close(fig)


def plot_dual(cycle_df, alias, output_path):
    fig, ax1 = plt.subplots(figsize=(15, 5.5))
    ax2 = ax1.twinx()
    draw_pulses(ax1, nonzero_points(cycle_df, "total_items"), "total_items", color="#d1495b")
    draw_pulses(ax2, nonzero_points(cycle_df, "splitter_cycles"), "splitter_cycles", color="#2b59c3")

    ax1.set_title(f"{alias} pulse load overview")
    ax1.set_xlabel("cycle")
    ax1.set_ylabel("items", color="#d1495b")
    ax2.set_ylabel("splitter cycles", color="#2b59c3")
    ax1.tick_params(axis="y", colors="#d1495b")
    ax2.tick_params(axis="y", colors="#2b59c3")
    ax1.axhline(0, color="#8d8d8d", linewidth=0.8, alpha=0.8)
    finalize_axes(ax1)
    ax2.yaxis.set_major_locator(MaxNLocator(nbins=7, integer=True))
    ax2.spines["top"].set_visible(False)

    ax1.plot([], [], color="#d1495b", linewidth=1.4, label="items")
    ax2.plot([], [], color="#2b59c3", linewidth=1.4, label="splitter cycles")
    handles = [ax1.lines[-1], ax2.lines[-1]]
    ax1.legend(handles, ["items", "splitter cycles"], loc="upper right", frameon=True)

    fig.tight_layout()
    fig.savefig(output_path, dpi=220)
    plt.close(fig)


def plot_zoom(cycle_df, alias, output_path, zoom_points=None):
    zoom_df = select_zoom_window(cycle_df, zoom_points=zoom_points)
    fig, ax1 = plt.subplots(figsize=(15, 6))
    draw_pulses(ax1, nonzero_points(zoom_df, "total_items"), "total_items", color="#d1495b")

    max_row = zoom_df.loc[zoom_df["total_items"].idxmax()]
    min_row = zoom_df.loc[zoom_df["total_items"].idxmin()]
    ax1.annotate(
        f"max {int(max_row['total_items'])}",
        xy=(max_row["cycle"], max_row["total_items"]),
        xytext=(10, 12),
        textcoords="offset points",
        color="#8f1020",
        fontsize=11,
    )
    ax1.annotate(
        f"min {int(min_row['total_items'])}",
        xy=(min_row["cycle"], min_row["total_items"]),
        xytext=(10, -18),
        textcoords="offset points",
        color="#495057",
        fontsize=11,
    )

    cycle_lo = int(zoom_df["cycle"].min())
    cycle_hi = int(zoom_df["cycle"].max())
    item_range = int(zoom_df["total_items"].max() - zoom_df["total_items"].min())

    ax1.set_title(f"{alias} hotspot zoom [{cycle_lo}, {cycle_hi}] range={item_range}")
    ax1.set_xlabel("cycle")
    ax1.set_ylabel("items", color="#d1495b")
    ax1.tick_params(axis="y", colors="#d1495b")
    ax1.axhline(0, color="#8d8d8d", linewidth=0.8, alpha=0.8)
    finalize_axes(ax1)

    fig.tight_layout()
    fig.savefig(output_path, dpi=220)
    plt.close(fig)


def analyze_table(conn, table_name, trap_df, coreid_variants, delta_limit, out_dir, zoom_points):
    alias = simplify_table_name(table_name)
    elem_df = load_elem_counts(conn, table_name, coreid_variants)
    cycle_df = build_cycle_dataframe(trap_df, elem_df, delta_limit)

    csv_path = out_dir / f"{alias}.csv"
    cycle_df.to_csv(csv_path, index=False)

    plot_items(cycle_df, alias, out_dir / f"{alias}-items.png")
    plot_cycles(cycle_df, alias, out_dir / f"{alias}-cycles.png")
    plot_dual(cycle_df, alias, out_dir / f"{alias}-both.png")
    plot_zoom(cycle_df, alias, out_dir / f"{alias}-zoom.png", zoom_points=zoom_points)

    return {
        "table": table_name,
        "alias": alias,
        "cycles": int(len(cycle_df)),
        "total_items": int(cycle_df["total_items"].sum()),
        "total_splitter_cycles": int(cycle_df["splitter_cycles"].sum()),
        "csv": csv_path,
    }


def main():
    parser = argparse.ArgumentParser(description="Analyze all *Elem tables and export per-cycle load CSV/plots.")
    parser.add_argument("db", help="Path to the SQLite database")
    parser.add_argument("--coreid", default="0", help="Core ID to analyze, accepts decimal or hex")
    parser.add_argument("--delta-limit", type=int, default=DEFAULT_DELTA_LIMIT, help="DeltaSplitter throughput")
    parser.add_argument("--zoom-points", type=int, help="Number of cycle points to include in zoom view")
    parser.add_argument(
        "--out-dir",
        help="Output directory (default: <db_stem>_elem_load)",
    )
    args = parser.parse_args()

    db_path = Path(args.db).resolve()
    if not db_path.exists():
        raise FileNotFoundError(db_path)

    out_dir = Path(args.out_dir) if args.out_dir else Path(f"{db_path.stem}_elem_load")
    out_dir.mkdir(parents=True, exist_ok=True)

    configure_plot_style()
    coreid_variants = format_coreid_variants(args.coreid)

    conn = sqlite3.connect(db_path)
    try:
        trap_df = load_trap_steps(conn, coreid_variants)
        elem_tables = list_elem_tables(conn)
        if not elem_tables:
            raise ValueError("No *Elem tables found in the database")

        results = []
        for table_name in elem_tables:
            results.append(
                analyze_table(
                    conn,
                    table_name,
                    trap_df,
                    coreid_variants,
                    args.delta_limit,
                    out_dir,
                    args.zoom_points,
                )
            )
    finally:
        conn.close()

    print(f"Output directory: {out_dir.resolve()}")
    for result in results:
        print(
            f"{result['table']} -> {result['alias']}: "
            f"cycles={result['cycles']}, items={result['total_items']}, "
            f"splitter_cycles={result['total_splitter_cycles']}"
        )


if __name__ == "__main__":
    sys.exit(main())
