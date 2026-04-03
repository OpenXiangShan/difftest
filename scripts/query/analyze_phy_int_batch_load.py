#!/usr/bin/env python3

import argparse
import sqlite3
import sys
from pathlib import Path

try:
    import matplotlib.pyplot as plt
    import pandas as pd
    import seaborn as sns
    from matplotlib.lines import Line2D
    from matplotlib.ticker import MaxNLocator
    from matplotlib.transforms import blended_transform_factory
except ModuleNotFoundError as exc:
    raise SystemExit(
        "Missing Python package: "
        f"{exc.name}. Use the repo venv: .venv-elem-load/bin/python "
        "difftest/scripts/query/analyze_phy_int_batch_load.py ..."
    )


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


def draw_pulses(ax, df, y_col, color):
    if df.empty:
        return
    ax.vlines(
        df["cycle"],
        0,
        df[y_col],
        color=color,
        linewidth=pulse_linewidth(len(df)),
        alpha=0.95,
    )


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
        [{"step": int(step), "trap_cycle": int(cycle)} for step, cycle in sorted(step_to_cycle.items())]
    )


def load_phy_int_batch_rows(conn):
    query = """
        SELECT ID, STEP, PhyIntRegStateElem
        FROM BatchStep
        ORDER BY STEP, ID
    """
    df = pd.read_sql_query(query, conn)
    if df.empty:
        raise ValueError("BatchStep is empty")
    df["step"] = df["STEP"].astype(int)
    df["batch_row_id"] = df["ID"].astype(int)
    df["phy_int_items"] = df["PhyIntRegStateElem"].astype(int)
    return df[["step", "batch_row_id", "phy_int_items"]]


def build_expanded_trace(trap_df, batch_df):
    merged = trap_df.merge(batch_df, on="step", how="inner")
    if merged.empty:
        raise ValueError("No BatchStep rows matched TrapEvent steps for the selected core")

    merged = merged.sort_values(["step", "batch_row_id"]).copy()
    merged["batch_index"] = merged.groupby("step").cumcount()
    merged["cycle"] = merged["trap_cycle"] + merged["batch_index"]
    return merged[["step", "trap_cycle", "batch_row_id", "batch_index", "cycle", "phy_int_items"]]


def build_collapsed_trace(expanded_df):
    collapsed_df = (
        expanded_df.groupby(["step", "trap_cycle"], as_index=False)
        .agg(
            total_items=("phy_int_items", "sum"),
            batch_records=("phy_int_items", "size"),
            nonzero_records=("phy_int_items", lambda s: int((s > 0).sum())),
        )
        .rename(columns={"trap_cycle": "cycle"})
        .sort_values(["step", "cycle"])
    )
    return collapsed_df[["step", "cycle", "total_items", "batch_records", "nonzero_records"]]


def build_cycle_dataframe(expanded_df):
    cycle_df = (
        expanded_df.groupby("cycle", as_index=False)
        .agg(
            total_items=("phy_int_items", "sum"),
            batch_records=("phy_int_items", "size"),
            nonzero_records=("phy_int_items", lambda s: int((s > 0).sum())),
        )
        .sort_values("cycle")
    )
    return cycle_df


def select_zoom_step(collapsed_df):
    candidates = collapsed_df[
        (collapsed_df["batch_records"] > 1) & (collapsed_df["total_items"] > 0)
    ].copy()
    if candidates.empty:
        return None
    candidates = candidates.sort_values(
        ["batch_records", "total_items", "nonzero_records", "cycle"],
        ascending=[False, False, False, True],
    )
    return candidates.iloc[0]


def add_peak_comparison(ax, single_cycle_df, multi_cycle_df):
    single_peak = int(single_cycle_df["total_items"].max()) if not single_cycle_df.empty else 0
    multi_peak = int(multi_cycle_df["phy_int_items"].max()) if not multi_cycle_df.empty else 0
    if single_peak <= 0 and multi_peak <= 0:
        return

    single_color = "#d8a452"
    multi_color = "#1f5aa6"
    xmin = 0.76
    xmax = 0.91
    label_x = 0.915
    arrow_x = 0.965
    text_transform = blended_transform_factory(ax.transAxes, ax.transData)

    ax.axhline(single_peak, xmin=xmin, xmax=xmax, color=single_color, linewidth=1.4, linestyle=(0, (7, 4)), alpha=0.95)
    ax.axhline(multi_peak, xmin=xmin, xmax=xmax, color=multi_color, linewidth=1.4, linestyle=(0, (7, 4)), alpha=0.95)

    ax.text(
        label_x,
        single_peak,
        f"single peak={single_peak}",
        transform=text_transform,
        color=single_color,
        fontsize=9.2,
        ha="left",
        va="center",
        bbox={
            "boxstyle": "round,pad=0.16",
            "facecolor": "#fffdf8",
            "edgecolor": single_color,
            "linewidth": 0.6,
            "alpha": 0.9,
        },
    )
    ax.text(
        label_x,
        multi_peak,
        f"multi peak={multi_peak}",
        transform=text_transform,
        color=multi_color,
        fontsize=9.2,
        ha="left",
        va="center",
        bbox={
            "boxstyle": "round,pad=0.16",
            "facecolor": "#fffdf8",
            "edgecolor": multi_color,
            "linewidth": 0.6,
            "alpha": 0.9,
        },
    )

    low_peak = min(single_peak, multi_peak)
    high_peak = max(single_peak, multi_peak)
    ratio = high_peak / max(1, low_peak)
    delta = abs(single_peak - multi_peak)

    ax.annotate(
        "",
        xy=(arrow_x, high_peak),
        xytext=(arrow_x, low_peak),
        xycoords=("axes fraction", "data"),
        textcoords=("axes fraction", "data"),
        arrowprops={
            "arrowstyle": "<->",
            "color": "#4a4a4a",
            "linewidth": 1.4,
            "shrinkA": 0,
            "shrinkB": 0,
        },
        annotation_clip=False,
    )
    ax.text(
        0.972,
        (single_peak + multi_peak) / 2,
        f"delta={delta}, ratio={ratio:.2f}x",
        transform=text_transform,
        color="#333333",
        fontsize=9.6,
        fontweight="bold",
        ha="left",
        va="center",
        bbox={
            "boxstyle": "round,pad=0.2",
            "facecolor": "#fffdf8",
            "edgecolor": "#8d8d8d",
            "linewidth": 0.6,
            "alpha": 0.92,
        },
    )


def plot_overlay(collapsed_df, expanded_df, output_path):
    fig, ax = plt.subplots(figsize=(15, 5.5))
    collapsed_points = collapsed_df[collapsed_df["total_items"] > 0]
    expanded_points = expanded_df[expanded_df["phy_int_items"] > 0]

    draw_pulses(ax, collapsed_points, "total_items", color="#d8a452")
    draw_pulses(ax, expanded_points, "phy_int_items", color="#1f5aa6")

    add_peak_comparison(ax, collapsed_points, expanded_points)

    ax.set_title("phy_int load balancing: single-cycle handling vs multi-cycle handling")
    ax.set_xlabel("cycle")
    ax.set_ylabel("items")
    ax.axhline(0, color="#8d8d8d", linewidth=0.8, alpha=0.8)
    finalize_axes(ax)

    handles = [
        Line2D([0], [0], color="#d8a452", linewidth=1.5, label="Single-Cycle Handling"),
        Line2D([0], [0], color="#1f5aa6", linewidth=1.5, label="Multi-Cycle Handling"),
    ]
    ax.legend(handles=handles, loc="upper right", frameon=True)

    ax.text(
        0.012,
        0.94,
        (
            f"collapsed points={len(collapsed_points)}  peak={int(collapsed_points['total_items'].max()) if not collapsed_points.empty else 0}\n"
            f"expanded points={len(expanded_points)}  peak={int(expanded_points['phy_int_items'].max()) if not expanded_points.empty else 0}"
        ),
        transform=ax.transAxes,
        fontsize=9.8,
        ha="left",
        va="top",
        color="#404040",
        bbox={
            "boxstyle": "round,pad=0.18",
            "facecolor": "#fffdf8",
            "edgecolor": "#8d8d8d",
            "linewidth": 0.7,
            "alpha": 0.9,
        },
    )

    fig.tight_layout()
    fig.savefig(output_path, dpi=220)
    plt.close(fig)


def plot_zoom(collapsed_df, expanded_df, output_path):
    zoom_row = select_zoom_step(collapsed_df)
    if zoom_row is None:
        return None

    step = int(zoom_row["step"])
    trap_cycle = int(zoom_row["cycle"])
    batch_records = int(zoom_row["batch_records"])
    total_items = int(zoom_row["total_items"])
    left_pad = 4
    right_pad = max(6, batch_records + 3)
    cycle_lo = max(0, trap_cycle - left_pad)
    cycle_hi = trap_cycle + right_pad

    collapsed_zoom = collapsed_df[
        (collapsed_df["cycle"] >= cycle_lo) & (collapsed_df["cycle"] <= cycle_hi)
    ].copy()
    expanded_zoom = expanded_df[
        (expanded_df["cycle"] >= cycle_lo) & (expanded_df["cycle"] <= cycle_hi)
    ].copy()

    fig, ax = plt.subplots(figsize=(13.5, 5.2))
    collapsed_points = collapsed_zoom[collapsed_zoom["total_items"] > 0]
    expanded_points = expanded_zoom[expanded_zoom["phy_int_items"] > 0]

    draw_pulses(ax, collapsed_points, "total_items", color="#d8a452")
    draw_pulses(ax, expanded_points, "phy_int_items", color="#1f5aa6")

    peak = max(
        int(collapsed_points["total_items"].max()) if not collapsed_points.empty else 0,
        int(expanded_points["phy_int_items"].max()) if not expanded_points.empty else 0,
        1,
    )

    ax.axvspan(trap_cycle - 0.45, trap_cycle + 0.45, color="#d8a452", alpha=0.12, zorder=0)
    ax.axvspan(trap_cycle - 0.45, trap_cycle + batch_records - 0.55, color="#1f5aa6", alpha=0.08, zorder=0)
    ax.axvline(trap_cycle, color="#8d8d8d", linewidth=1.0, linestyle=(0, (3, 3)), alpha=0.8)

    ax.set_xlim(cycle_lo, cycle_hi)
    ax.set_ylim(0, int(peak * 1.28) + 1)
    ax.axhline(0, color="#8d8d8d", linewidth=0.8, alpha=0.8)
    ax.set_title("Zoom: one phy_int point split across multiple batch cycles")
    ax.set_xlabel("cycle")
    ax.set_ylabel("items")
    finalize_axes(ax)

    handles = [
        Line2D([0], [0], color="#d8a452", linewidth=1.5, label="Single-Cycle Handling"),
        Line2D([0], [0], color="#1f5aa6", linewidth=1.5, label="Multi-Cycle Handling"),
    ]
    ax.legend(handles=handles, loc="upper right", frameon=True)

    ax.annotate(
        f"collapsed point\nstep={step}\ncycle={trap_cycle}\nitems={total_items}",
        xy=(trap_cycle, total_items),
        xytext=(trap_cycle - max(3, batch_records // 4), peak * 1.12),
        textcoords="data",
        fontsize=9.8,
        color="#8a5a00",
        ha="center",
        va="top",
        arrowprops={
            "arrowstyle": "->",
            "color": "#8a5a00",
            "linewidth": 1.0,
        },
        bbox={
            "boxstyle": "round,pad=0.2",
            "facecolor": "#fffdf8",
            "edgecolor": "#d8a452",
            "linewidth": 0.7,
            "alpha": 0.92,
        },
    )

    ax.annotate(
        f"expanded into {batch_records} batch cycles",
        xy=(trap_cycle + batch_records - 1, max(1, int(expanded_points['phy_int_items'].max()) if not expanded_points.empty else 1)),
        xytext=(trap_cycle + batch_records * 0.52, peak * 1.22),
        textcoords="data",
        fontsize=9.8,
        color="#1f5aa6",
        ha="center",
        va="top",
        arrowprops={
            "arrowstyle": "->",
            "color": "#1f5aa6",
            "linewidth": 1.0,
        },
        bbox={
            "boxstyle": "round,pad=0.2",
            "facecolor": "#fffdf8",
            "edgecolor": "#1f5aa6",
            "linewidth": 0.7,
            "alpha": 0.92,
        },
    )

    fig.tight_layout()
    fig.savefig(output_path, dpi=220)
    plt.close(fig)
    return {
        "step": step,
        "cycle": trap_cycle,
        "batch_records": batch_records,
        "total_items": total_items,
        "cycle_lo": cycle_lo,
        "cycle_hi": cycle_hi,
    }


def main():
    parser = argparse.ArgumentParser(
        description="Expand BatchStep.PhyIntRegStateElem into consecutive per-cycle pulses based on TrapEvent cycles."
    )
    parser.add_argument("db", help="Path to the SQLite database")
    parser.add_argument("--coreid", default="0", help="Core ID to analyze, accepts decimal or hex")
    parser.add_argument(
        "--out-dir",
        help="Output directory (default: <db_stem>_phy_int_batch)",
    )
    args = parser.parse_args()

    db_path = Path(args.db).resolve()
    if not db_path.exists():
        raise FileNotFoundError(db_path)

    out_dir = Path(args.out_dir) if args.out_dir else Path(f"{db_path.stem}_phy_int_batch")
    out_dir.mkdir(parents=True, exist_ok=True)

    configure_plot_style()
    coreid_variants = format_coreid_variants(args.coreid)

    conn = sqlite3.connect(db_path)
    try:
        trap_df = load_trap_steps(conn, coreid_variants)
        batch_df = load_phy_int_batch_rows(conn)
        expanded_df = build_expanded_trace(trap_df, batch_df)
        collapsed_df = build_collapsed_trace(expanded_df)
        cycle_df = build_cycle_dataframe(expanded_df)
    finally:
        conn.close()

    collapsed_csv_path = out_dir / "phy_int_trap.csv"
    trace_csv_path = out_dir / "phy_int_batch_trace.csv"
    cycle_csv_path = out_dir / "phy_int_batch.csv"
    plot_path = out_dir / "phy_int_batch-overlay.png"
    zoom_plot_path = out_dir / "phy_int_batch-zoom.png"

    collapsed_df.to_csv(collapsed_csv_path, index=False)
    expanded_df.to_csv(trace_csv_path, index=False)
    cycle_df.to_csv(cycle_csv_path, index=False)
    plot_overlay(collapsed_df, expanded_df, plot_path)
    zoom_info = plot_zoom(collapsed_df, expanded_df, zoom_plot_path)

    print(f"Output directory: {out_dir.resolve()}")
    print(f"Collapsed TrapEvent CSV: {collapsed_csv_path.resolve()}")
    print(f"Expanded trace CSV: {trace_csv_path.resolve()}")
    print(f"Expanded cycle CSV: {cycle_csv_path.resolve()}")
    print(f"Overlay plot: {plot_path.resolve()}")
    if zoom_info is not None:
        print(
            f"Zoom plot: {zoom_plot_path.resolve()} "
            f"(step={zoom_info['step']}, cycle={zoom_info['cycle']}, "
            f"batch_records={zoom_info['batch_records']}, window=[{zoom_info['cycle_lo']}, {zoom_info['cycle_hi']}])"
        )
    print(
        f"steps={expanded_df['step'].nunique()}, batch_rows={len(expanded_df)}, "
        f"cycles={len(cycle_df)}, total_items={int(cycle_df['total_items'].sum())}, "
        f"peak_items={int(cycle_df['total_items'].max())}"
    )


if __name__ == "__main__":
    sys.exit(main())
