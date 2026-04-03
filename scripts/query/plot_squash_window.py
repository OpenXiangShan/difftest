#!/usr/bin/env python3

import argparse
import math
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
        "difftest/scripts/query/plot_squash_window.py ..."
    )


DEFAULT_COLUMN = "total_items"
DEFAULT_WINDOW = 512


def parse_args():
    parser = argparse.ArgumentParser(
        description="Visualize squash imbalance within a selected cycle window.",
    )
    parser.add_argument("dense_csv", help="CSV expected to have dense pulses, e.g. EBIDU phy_int.csv")
    parser.add_argument("sparse_csv", help="CSV expected to have sparse pulses, e.g. EBISDU phy_int.csv")
    parser.add_argument(
        "--column",
        default=DEFAULT_COLUMN,
        help=f"Y-axis column to plot (default: {DEFAULT_COLUMN})",
    )
    parser.add_argument(
        "--window",
        type=int,
        default=DEFAULT_WINDOW,
        help=f"Cycle window size for auto selection (default: {DEFAULT_WINDOW})",
    )
    parser.add_argument(
        "--start",
        type=int,
        help="Window start cycle; if omitted the script will auto-select a contrastive interval",
    )
    parser.add_argument(
        "--end",
        type=int,
        help="Window end cycle (exclusive); required together with --start for manual selection",
    )
    parser.add_argument(
        "--labels",
        nargs=2,
        metavar=("DENSE", "SPARSE"),
        help="Legend labels for the two CSV files",
    )
    parser.add_argument(
        "-o",
        "--output",
        help="Output PNG path (default: <dense_csv_stem>-squash-window.png)",
    )
    parser.add_argument(
        "--title",
        help="Optional plot title",
    )
    parser.add_argument(
        "--step",
        type=int,
        help="Auto-search step size in cycles (default: window // 8)",
    )
    return parser.parse_args()


def configure_plot_style():
    sns.set_theme(style="whitegrid", context="talk")
    plt.rcParams.update(
        {
            "figure.facecolor": "#f8f6f1",
            "axes.facecolor": "#fffdf8",
            "axes.edgecolor": "#5b5b5b",
            "grid.color": "#d8d2c6",
            "grid.alpha": 0.55,
            "axes.titleweight": "bold",
            "axes.labelweight": "medium",
            "savefig.facecolor": "#f8f6f1",
        }
    )


def finalize_axes(ax):
    ax.xaxis.set_major_locator(MaxNLocator(nbins=8, integer=True))
    ax.yaxis.set_major_locator(MaxNLocator(nbins=6, integer=True))
    ax.margins(x=0.01)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)


def load_series(csv_path, column, label):
    path = Path(csv_path).resolve()
    if not path.exists():
        raise FileNotFoundError(path)

    df = pd.read_csv(path)
    required = {"cycle", column}
    missing = required - set(df.columns)
    if missing:
        raise ValueError(f"{path} missing columns: {', '.join(sorted(missing))}")

    data = df[["cycle", column]].copy()
    data["cycle"] = pd.to_numeric(data["cycle"], errors="raise").astype(int)
    data[column] = pd.to_numeric(data[column], errors="coerce").fillna(0).astype(int)
    data = data.groupby("cycle", as_index=False)[column].sum().sort_values("cycle")
    active = data[data[column] > 0].copy()

    return {
        "path": path,
        "label": label,
        "data": data,
        "active": active,
        "peak": int(data[column].max()) if not data.empty else 0,
        "active_cycles": active["cycle"].astype(int).to_numpy(),
        "active_values": active[column].astype(int).to_numpy(),
    }


def default_labels(dense_csv, sparse_csv, labels):
    if labels:
        return labels
    return (Path(dense_csv).parent.name, Path(sparse_csv).parent.name)


def default_output_path(csv_path):
    path = Path(csv_path).resolve()
    return path.with_name(f"{path.stem}-squash-window.png")


def auto_select_window(dense, sparse, window, step):
    max_cycle = int(max(dense["data"]["cycle"].max(), sparse["data"]["cycle"].max()) + 1)
    dense_cycles = dense["active_cycles"]
    dense_values = dense["active_values"]
    sparse_cycles = sparse["active_cycles"]
    sparse_values = sparse["active_values"]

    best = None
    for start in range(0, max_cycle, step):
        end = start + window
        d0 = dense_cycles.searchsorted(start, side="left")
        d1 = dense_cycles.searchsorted(end, side="left")
        s0 = sparse_cycles.searchsorted(start, side="left")
        s1 = sparse_cycles.searchsorted(end, side="left")

        dense_count = int(d1 - d0)
        sparse_count = int(s1 - s0)
        dense_occupancy = dense_count / max(1, window)
        sparse_occupancy = sparse_count / max(1, window)

        if dense_occupancy < 0.55 or sparse_occupancy > 0.25 or sparse_count < 2:
            continue

        dense_slice = dense_values[d0:d1]
        sparse_slice = sparse_values[s0:s1]
        dense_peak = int(dense_slice.max()) if len(dense_slice) else 0
        sparse_peak = int(sparse_slice.max()) if len(sparse_slice) else 0
        if dense_peak < 6 or sparse_peak < 6:
            continue

        peak_ratio = max(dense_peak, sparse_peak) / max(1, min(dense_peak, sparse_peak))
        if peak_ratio > 1.8:
            continue

        peak_anchor = min(dense_peak, sparse_peak)
        large_threshold = max(4, int(round(peak_anchor * 0.7)))
        dense_large = int((dense_slice >= large_threshold).sum())
        sparse_large = int((sparse_slice >= large_threshold).sum())
        if dense_large < 3:
            continue

        score = (
            -abs(dense_peak - sparse_peak),
            dense_count - sparse_count,
            dense_large,
            -sparse_count,
            dense_peak + sparse_peak,
        )
        candidate = {
            "start": start,
            "end": end,
            "dense_count": dense_count,
            "sparse_count": sparse_count,
            "dense_peak": dense_peak,
            "sparse_peak": sparse_peak,
            "dense_large": dense_large,
            "sparse_large": sparse_large,
            "score": score,
        }
        if best is None or candidate["score"] > best["score"]:
            best = candidate

    if best is None:
        raise ValueError("No contrastive window found; try a larger --window or manual --start/--end")
    return best


def subset_window(dataset, start, end, column):
    window_df = dataset["data"][(dataset["data"]["cycle"] >= start) & (dataset["data"]["cycle"] < end)].copy()
    active = window_df[window_df[column] > 0].copy()
    active_cycles = active["cycle"].to_list()
    gaps = [b - a for a, b in zip(active_cycles, active_cycles[1:])]
    return {
        "window_df": window_df,
        "active": active,
        "count": int(len(active)),
        "sum": int(window_df[column].sum()) if not window_df.empty else 0,
        "peak": int(window_df[column].max()) if not window_df.empty else 0,
        "occupancy": float(len(active)) / max(1, end - start),
        "mean_gap": float(sum(gaps) / len(gaps)) if gaps else math.nan,
    }


def format_gap(value):
    if math.isnan(value):
        return "N/A"
    return f"{value:.1f}"


def plot_overlay_window(ax, dense_stats, sparse_stats, dense_label, sparse_label, column, start, end, global_peak):
    dense_active = dense_stats["active"]
    sparse_active = sparse_stats["active"]
    dense_color = "#d2a45a"
    sparse_color = "#0b4ea2"

    if not dense_active.empty:
        ax.vlines(
            dense_active["cycle"],
            0,
            dense_active[column],
            color=dense_color,
            linewidth=0.95,
            alpha=0.58,
            zorder=2,
        )
    if not sparse_active.empty:
        ax.vlines(
            sparse_active["cycle"],
            0,
            sparse_active[column],
            color="#eef4ff",
            linewidth=2.4,
            alpha=0.92,
            zorder=3,
        )
        ax.vlines(
            sparse_active["cycle"],
            0,
            sparse_active[column],
            color=sparse_color,
            linewidth=1.55,
            alpha=1.0,
            zorder=4,
        )

    ax.set_xlim(start, end)
    ax.set_ylim(0, max(2, int(math.ceil(global_peak * 1.08))))
    ax.axhline(0, color="#8d8d8d", linewidth=0.8, alpha=0.8)
    ax.set_ylabel(column)
    ax.set_xlabel("cycle")

    dense_text = (
        f"{dense_label}: active={dense_stats['count']}  "
        f"occupancy={dense_stats['occupancy'] * 100:.1f}%  "
        f"peak={dense_stats['peak']}  "
        f"large={int((dense_stats['active'][column] >= max(4, int(round(max(1, dense_stats['peak']) * 0.7)))).sum())}  "
        f"mean_gap={format_gap(dense_stats['mean_gap'])}"
    )
    sparse_text = (
        f"{sparse_label}: active={sparse_stats['count']}  "
        f"occupancy={sparse_stats['occupancy'] * 100:.1f}%  "
        f"peak={sparse_stats['peak']}  "
        f"large={int((sparse_stats['active'][column] >= max(4, int(round(max(1, sparse_stats['peak']) * 0.7)))).sum())}  "
        f"mean_gap={format_gap(sparse_stats['mean_gap'])}"
    )
    ax.text(
        0.012,
        0.95,
        dense_text,
        transform=ax.transAxes,
        fontsize=10.2,
        ha="left",
        va="top",
        color="#404040",
        bbox={
            "boxstyle": "round,pad=0.18",
            "facecolor": "#fffdf8",
            "edgecolor": dense_color,
            "linewidth": 0.7,
            "alpha": 0.88,
        },
    )
    ax.text(
        0.012,
        0.865,
        sparse_text,
        transform=ax.transAxes,
        fontsize=10.2,
        ha="left",
        va="top",
        color="#404040",
        bbox={
            "boxstyle": "round,pad=0.18",
            "facecolor": "#fffdf8",
            "edgecolor": sparse_color,
            "linewidth": 0.7,
            "alpha": 0.88,
        },
    )
    finalize_axes(ax)


def make_title(args, start, end, dense_stats, sparse_stats, dense_label, sparse_label):
    if args.title:
        return args.title
    density_ratio = float("inf") if sparse_stats["count"] == 0 else dense_stats["count"] / sparse_stats["count"]
    peak_ratio = (
        float("inf")
        if min(dense_stats["peak"], sparse_stats["peak"]) == 0
        else max(dense_stats["peak"], sparse_stats["peak"]) / min(dense_stats["peak"], sparse_stats["peak"])
    )
    return (
        f"Squash contrast on {args.column} [{start}, {end})  "
        f"active ratio={density_ratio:.1f}x  peak ratio={peak_ratio:.2f}x"
    )


def main():
    args = parse_args()
    if (args.start is None) != (args.end is None):
        raise SystemExit("--start and --end must be provided together")
    if args.start is not None and args.end <= args.start:
        raise SystemExit("--end must be greater than --start")

    dense_label, sparse_label = default_labels(args.dense_csv, args.sparse_csv, args.labels)
    dense = load_series(args.dense_csv, args.column, dense_label)
    sparse = load_series(args.sparse_csv, args.column, sparse_label)

    step = args.step if args.step is not None else max(1, args.window // 8)
    if args.start is None:
        selected = auto_select_window(dense, sparse, args.window, step)
        start = selected["start"]
        end = selected["end"]
        selection_note = (
            f"Auto-selected [{start}, {end}) with similar peaks and strong density contrast: "
            f"{dense_label} active={selected['dense_count']}, peak={selected['dense_peak']}; "
            f"{sparse_label} active={selected['sparse_count']}, peak={selected['sparse_peak']}"
        )
    else:
        start, end = args.start, args.end
        selection_note = f"Manual window [{start}, {end})"

    dense_stats = subset_window(dense, start, end, args.column)
    sparse_stats = subset_window(sparse, start, end, args.column)
    global_peak = max(dense_stats["peak"], sparse_stats["peak"], 1)

    output_path = Path(args.output).resolve() if args.output else default_output_path(args.dense_csv)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    configure_plot_style()
    fig, ax = plt.subplots(figsize=(15, 6.1))
    plot_overlay_window(
        ax,
        dense_stats,
        sparse_stats,
        dense_label,
        sparse_label,
        args.column,
        start,
        end,
        global_peak,
    )

    fig.suptitle(make_title(args, start, end, dense_stats, sparse_stats, dense_label, sparse_label), y=0.97)
    fig.text(0.075, 0.925, selection_note, fontsize=10, color="#5b5b5b", ha="left")
    fig.text(
        0.075,
        0.04,
        "Dense orange pulses are drawn first; sparse blue pulses are drawn after to avoid being hidden and to emphasize squash-filtered sparsity.",
        fontsize=9.5,
        color="#5b5b5b",
        ha="left",
    )

    fig.subplots_adjust(left=0.08, right=0.985, top=0.88, bottom=0.14)
    fig.savefig(output_path, dpi=220)
    plt.close(fig)

    print(f"Saved plot: {output_path}")
    print(selection_note)
    print(
        f"{dense_label}: active={dense_stats['count']}, occupancy={dense_stats['occupancy'] * 100:.2f}%, "
        f"peak={dense_stats['peak']}, sum={dense_stats['sum']}, mean_gap={format_gap(dense_stats['mean_gap'])}"
    )
    print(
        f"{sparse_label}: active={sparse_stats['count']}, occupancy={sparse_stats['occupancy'] * 100:.2f}%, "
        f"peak={sparse_stats['peak']}, sum={sparse_stats['sum']}, mean_gap={format_gap(sparse_stats['mean_gap'])}"
    )


if __name__ == "__main__":
    sys.exit(main())
