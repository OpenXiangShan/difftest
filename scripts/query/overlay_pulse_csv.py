#!/usr/bin/env python3

import argparse
import math
import sys
from collections import defaultdict
from pathlib import Path

try:
    import matplotlib.pyplot as plt
    import pandas as pd
    import seaborn as sns
    from matplotlib.lines import Line2D
    from matplotlib.patches import ConnectionPatch
    from matplotlib.ticker import MaxNLocator
    from matplotlib.transforms import blended_transform_factory
except ModuleNotFoundError as exc:
    raise SystemExit(
        "Missing Python package: "
        f"{exc.name}. Use the repo venv: .venv-elem-load/bin/python "
        "difftest/scripts/query/overlay_pulse_csv.py ..."
    )


DEFAULT_COLUMN = "total_items"


def parse_args():
    parser = argparse.ArgumentParser(
        description="Overlay pulse plots from multiple CSV files on the same figure.",
    )
    parser.add_argument(
        "csvs",
        nargs="+",
        help="Input CSV files with at least 'cycle' and the selected y-axis column",
    )
    parser.add_argument(
        "--column",
        default=DEFAULT_COLUMN,
        help=f"Y-axis column to plot (default: {DEFAULT_COLUMN})",
    )
    parser.add_argument(
        "--labels",
        nargs="+",
        help="Optional legend labels, must have the same count as input CSV files",
    )
    parser.add_argument(
        "-o",
        "--output",
        help="Output PNG path (default: <first_csv_stem>-overlay-<column>.png)",
    )
    parser.add_argument(
        "--title",
        help="Plot title (default: generated from the selected column)",
    )
    parser.add_argument(
        "--front",
        choices=("short", "long"),
        default="short",
        help="Which pulse height should stay visually in front when cycles overlap",
    )
    parser.add_argument(
        "--figsize",
        nargs=2,
        type=float,
        default=(15, 5.5),
        metavar=("WIDTH", "HEIGHT"),
        help="Figure size in inches (default: 15 5.5)",
    )
    parser.add_argument(
        "--dpi",
        type=int,
        default=220,
        help="Output image DPI (default: 220)",
    )
    parser.add_argument(
        "--alpha",
        type=float,
        default=0.92,
        help="Pulse alpha value (default: 0.92)",
    )
    parser.add_argument(
        "--y-break",
        choices=("auto", "off"),
        default="auto",
        help="Automatically fold the y-axis when there is a strong outlier (default: auto)",
    )
    parser.add_argument(
        "--overlap-spread",
        type=float,
        default=1.0,
        help="Horizontal spread for pulses sharing the same cycle (default: 1.0)",
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


def pulse_linewidth(point_count, dataset_count):
    if dataset_count >= 8:
        return 1.2
    if dataset_count >= 5:
        return 1.5
    if point_count <= 64:
        return 2.6
    if point_count <= 256:
        return 2.1
    if point_count <= 1024:
        return 1.6
    return 1.2


def round_break_threshold(value):
    if value <= 20:
        step = 2
    elif value <= 100:
        step = 5
    else:
        step = 10
    return int(math.ceil(value / step) * step)


def load_series(csv_path, column, label):
    path = Path(csv_path).resolve()
    if not path.exists():
        raise FileNotFoundError(path)

    df = pd.read_csv(path)
    required_columns = {"cycle", column}
    missing = required_columns - set(df.columns)
    if missing:
        raise ValueError(f"{path} missing columns: {', '.join(sorted(missing))}")

    data = df[["cycle", column]].copy()
    data["cycle"] = pd.to_numeric(data["cycle"], errors="raise").astype(int)
    data[column] = pd.to_numeric(data[column], errors="coerce").fillna(0).astype(int)
    data = data.groupby("cycle", as_index=False)[column].sum().sort_values("cycle")

    points = data[data[column] > 0].rename(columns={column: "value"}).copy()
    peak = int(data[column].max()) if not data.empty else 0
    total = int(data[column].sum()) if not data.empty else 0
    cycle_count = int(len(data))
    point_count = int(len(points))
    return {
        "path": path,
        "label": label,
        "data": data,
        "points": points,
        "peak": peak,
        "total": total,
        "cycle_count": cycle_count,
        "point_count": point_count,
    }


def validate_labels(csvs, labels):
    if labels is None:
        return [Path(csv).stem for csv in csvs]
    if len(labels) != len(csvs):
        raise ValueError("--labels count must match the number of input CSV files")
    return labels


def default_output_path(first_csv, column):
    first_path = Path(first_csv).resolve()
    return first_path.with_name(f"{first_path.stem}-overlay-{column}.png")


def sort_datasets(datasets):
    return sorted(
        datasets,
        key=lambda item: (item["peak"], item["total"], item["cycle_count"], item["label"].lower()),
    )


def build_cycle_buckets(datasets, colors):
    cycle_buckets = defaultdict(list)
    dataset_count = len(datasets)

    for index, dataset in enumerate(datasets):
        width_scale = 1.0
        alpha = max(0.34, 0.9 - index * 0.22)
        outline_extra = max(0.7, 1.2 - index * 0.18)

        dataset["dataset_index"] = index
        dataset["color"] = colors[index]
        dataset["linewidth"] = pulse_linewidth(dataset["point_count"], dataset_count) * width_scale
        dataset["alpha"] = alpha
        dataset["outline_extra"] = outline_extra
        for row in dataset["points"].itertuples(index=False):
            cycle_buckets[int(row.cycle)].append(
                {
                    "value": int(row.value),
                    "color": dataset["color"],
                    "label": dataset["label"],
                    "linewidth": dataset["linewidth"],
                    "alpha": dataset["alpha"],
                    "outline_extra": dataset["outline_extra"],
                    "dataset_index": index,
                }
            )
    return cycle_buckets


def overlap_offsets(count, spread):
    if count <= 1:
        return [0.0]
    step = spread / (count - 1)
    start = -spread / 2
    return [start + step * index for index in range(count)]


def draw_overlay(ax, cycle_buckets, front, alpha, overlap_spread):
    short_in_front = front == "short"
    layered_points = defaultdict(lambda: {"x": [], "y": [], "style": None})

    for cycle in sorted(cycle_buckets):
        offset_order = sorted(cycle_buckets[cycle], key=lambda item: item["dataset_index"])
        offset_map = {
            item["dataset_index"]: offset
            for item, offset in zip(offset_order, overlap_offsets(len(offset_order), overlap_spread))
        }
        points = sorted(
            cycle_buckets[cycle],
            key=lambda item: (item["value"], item["dataset_index"]),
            reverse=short_in_front,
        )
        for z_offset, point in enumerate(points):
            layer = layered_points[(z_offset, point["dataset_index"])]
            layer["x"].append(cycle + offset_map[point["dataset_index"]])
            layer["y"].append(point["value"])
            layer["style"] = point

    dataset_indices = sorted({dataset_index for _z_offset, dataset_index in layered_points})
    front_dataset_index = dataset_indices[0] if short_in_front else dataset_indices[-1]

    def layer_sort_key(item):
        (z_offset, dataset_index), _layer = item
        is_front = dataset_index == front_dataset_index
        return (is_front, z_offset, dataset_index)

    for (z_offset, _dataset_index), layer in sorted(layered_points.items(), key=layer_sort_key):
        style = layer["style"]
        outline_extra = style["outline_extra"]
        outline_alpha = min(style["alpha"] + 0.08, 1.0)
        if style["dataset_index"] != front_dataset_index:
            outline_extra = max(0.55, outline_extra * 0.68)
            outline_alpha = min(outline_alpha, 0.44)

        ax.vlines(
            layer["x"],
            0,
            layer["y"],
            color="#fffdf8",
            linewidth=style["linewidth"] + outline_extra,
            alpha=outline_alpha,
            zorder=1.5 + z_offset,
            capstyle="butt",
        )
        ax.vlines(
            layer["x"],
            0,
            layer["y"],
            color=style["color"],
            linewidth=style["linewidth"],
            alpha=min(alpha, style["alpha"]),
            zorder=2.3 + z_offset,
            capstyle="butt",
        )


def build_legend_handles(datasets):
    return [
        Line2D([0], [0], color=dataset["color"], linewidth=2.4, label=dataset["label"])
        for dataset in datasets
    ]


def select_peak_comparison(datasets):
    candidates = [dataset for dataset in datasets if dataset["peak"] > 0]
    if len(candidates) < 2:
        return None

    low = min(candidates, key=lambda item: (item["peak"], item["dataset_index"]))
    high = max(candidates, key=lambda item: (item["peak"], item["dataset_index"]))
    if low["dataset_index"] == high["dataset_index"]:
        return None

    ratio = float("inf") if low["peak"] == 0 else high["peak"] / low["peak"]
    return {
        "low": low,
        "high": high,
        "ratio": ratio,
    }


def format_ratio_text(ratio):
    if not math.isfinite(ratio):
        return "inf x"
    if ratio >= 10:
        return f"{ratio:.1f}x"
    return f"{ratio:.2f}x"


def add_peak_line(ax, dataset, x_start=0.77, x_end=0.92, label_x=0.925):
    ax.axhline(
        dataset["peak"],
        xmin=x_start,
        xmax=x_end,
        color=dataset["color"],
        linewidth=1.7,
        linestyle=(0, (7, 4)),
        alpha=0.95,
        zorder=5.2,
    )
    text_transform = blended_transform_factory(ax.transAxes, ax.transData)
    ax.text(
        label_x,
        dataset["peak"],
        f"{dataset['label']} peak={dataset['peak']}",
        transform=text_transform,
        color=dataset["color"],
        fontsize=9.5,
        ha="left",
        va="center",
        bbox={
            "boxstyle": "round,pad=0.18",
            "facecolor": "#fffdf8",
            "edgecolor": dataset["color"],
            "linewidth": 0.6,
            "alpha": 0.88,
        },
    )


def annotate_peak_comparison_single(ax, datasets):
    comparison = select_peak_comparison(datasets)
    if comparison is None:
        return

    add_peak_line(ax, comparison["low"])
    add_peak_line(ax, comparison["high"])

    midpoint = (comparison["low"]["peak"] + comparison["high"]["peak"]) / 2
    text_transform = blended_transform_factory(ax.transAxes, ax.transData)
    ax.annotate(
        "",
        xy=(0.965, comparison["high"]["peak"]),
        xytext=(0.965, comparison["low"]["peak"]),
        xycoords=("axes fraction", "data"),
        textcoords=("axes fraction", "data"),
        arrowprops={
            "arrowstyle": "<->",
            "color": "#4a4a4a",
            "linewidth": 1.5,
            "shrinkA": 0,
            "shrinkB": 0,
        },
        annotation_clip=False,
    )
    ax.text(
        0.972,
        midpoint,
        format_ratio_text(comparison["ratio"]),
        transform=text_transform,
        color="#333333",
        fontsize=10,
        fontweight="bold",
        ha="left",
        va="center",
        bbox={
            "boxstyle": "round,pad=0.2",
            "facecolor": "#fffdf8",
            "edgecolor": "#8d8d8d",
            "linewidth": 0.6,
            "alpha": 0.9,
        },
    )


def annotate_peak_comparison_broken(fig, ax_upper, ax_lower, datasets, y_break):
    comparison = select_peak_comparison(datasets)
    if comparison is None:
        return

    low_ax = ax_lower if comparison["low"]["peak"] <= y_break["lower_top"] else ax_upper
    high_ax = ax_upper if comparison["high"]["peak"] >= y_break["upper_bottom"] else ax_lower

    if low_ax is high_ax:
        annotate_peak_comparison_single(low_ax, [comparison["low"], comparison["high"]])
        return

    add_peak_line(low_ax, comparison["low"])
    add_peak_line(high_ax, comparison["high"])

    connection = ConnectionPatch(
        xyA=(0.965, comparison["low"]["peak"]),
        coordsA=ax_lower.get_yaxis_transform(),
        xyB=(0.965, comparison["high"]["peak"]),
        coordsB=ax_upper.get_yaxis_transform(),
        arrowstyle="<->",
        linewidth=1.5,
        color="#4a4a4a",
        shrinkA=0,
        shrinkB=0,
    )
    fig.add_artist(connection)

    low_xy_fig = fig.transFigure.inverted().transform(
        ax_lower.get_yaxis_transform().transform((0.965, comparison["low"]["peak"]))
    )
    high_xy_fig = fig.transFigure.inverted().transform(
        ax_upper.get_yaxis_transform().transform((0.965, comparison["high"]["peak"]))
    )
    mid_x = max(low_xy_fig[0], high_xy_fig[0]) + 0.008
    mid_y = (low_xy_fig[1] + high_xy_fig[1]) / 2
    fig.text(
        mid_x,
        mid_y,
        format_ratio_text(comparison["ratio"]),
        color="#333333",
        fontsize=10,
        fontweight="bold",
        ha="left",
        va="center",
        bbox={
            "boxstyle": "round,pad=0.2",
            "facecolor": "#fffdf8",
            "edgecolor": "#8d8d8d",
            "linewidth": 0.6,
            "alpha": 0.9,
        },
    )


def cycles_match(datasets):
    if len(datasets) <= 1:
        return True
    baseline = tuple(datasets[0]["data"]["cycle"].tolist())
    return all(tuple(dataset["data"]["cycle"].tolist()) == baseline for dataset in datasets[1:])


def detect_y_break(datasets):
    best_break = None

    for dataset in datasets:
        series = pd.Series(dataset["points"]["value"].tolist(), dtype="float64")
        series = series[series > 0]
        if len(series) < 64:
            continue

        max_value = float(series.max())
        top_two = series.nlargest(min(2, len(series))).tolist()
        second_peak = float(top_two[-1]) if len(top_two) == 2 else 0.0
        q999 = float(series.quantile(0.999))
        q9995 = float(series.quantile(0.9995))
        q99995 = float(series.quantile(0.99995))
        percentile_candidates = [
            ("p99.9", q999),
            ("p99.95", q9995),
            ("p99.995", q99995),
        ]

        threshold_seed = max(
            q99995,
            q9995 + max(2.0, q9995 * 0.05),
            q999 + max(6.0, q999 * 0.25),
        )
        bulk_top = round_break_threshold(threshold_seed)
        upper_values = series[series > bulk_top]
        rare_count_limit = max(3, int(math.ceil(len(series) * 0.00006)))

        has_strong_outlier = (
            not upper_values.empty
            and len(upper_values) <= rare_count_limit
            and max_value >= max(bulk_top + 18, 1.55 * max(bulk_top, 1.0), 1.7 * max(second_peak, 1.0))
        )
        if not has_strong_outlier:
            continue

        upper_floor_candidate = int(math.floor(float(upper_values.min())))
        gap = upper_floor_candidate - bulk_top
        if gap < max(4, int(math.ceil(bulk_top * 0.05))):
            continue

        upper_bottom = max(bulk_top + 4, upper_floor_candidate - max(3, gap // 4))
        upper_top = int(math.ceil(max_value * 1.04))
        if upper_bottom >= upper_top:
            continue

        candidate_break = {
            "lower_top": bulk_top,
            "upper_bottom": upper_bottom,
            "upper_top": upper_top,
            "source_label": dataset["label"],
            "threshold_seed": threshold_seed,
            "percentiles": percentile_candidates,
        }
        if best_break is None or candidate_break["lower_top"] > best_break["lower_top"]:
            best_break = candidate_break

    return best_break


def draw_break_marks(ax_upper, ax_lower):
    kwargs = {"color": "#5b5b5b", "clip_on": False, "linewidth": 1.2}
    ax_upper.plot((-0.015, +0.015), (-0.02, +0.02), transform=ax_upper.transAxes, **kwargs)
    ax_upper.plot((0.985, 1.015), (-0.02, +0.02), transform=ax_upper.transAxes, **kwargs)
    ax_lower.plot((-0.015, +0.015), (0.98, 1.02), transform=ax_lower.transAxes, **kwargs)
    ax_lower.plot((0.985, 1.015), (0.98, 1.02), transform=ax_lower.transAxes, **kwargs)


def format_break_annotation(y_break):
    percentile_text = ", ".join(
        f"{label}={int(round(value))}" for label, value in y_break["percentiles"]
    )
    return (
        f"Auto break from {y_break['source_label']} by tail percentiles "
        f"({percentile_text}); folded above {y_break['lower_top']}"
    )


def plot_single_axis(ax, cycle_buckets, datasets, column, title, front, alpha, overlap_spread):
    draw_overlay(ax, cycle_buckets, front, alpha, overlap_spread)
    annotate_peak_comparison_single(ax, datasets)
    ax.set_title(title)
    ax.set_xlabel("cycle")
    ax.set_ylabel(column)
    ax.axhline(0, color="#8d8d8d", linewidth=0.8, alpha=0.8)
    finalize_axes(ax)
    ax.legend(handles=build_legend_handles(datasets), loc="upper right", frameon=True)


def plot_broken_axis(
    datasets,
    cycle_buckets,
    column,
    output_path,
    title,
    front,
    figsize,
    dpi,
    alpha,
    overlap_spread,
    y_break,
):
    fig, (ax_upper, ax_lower) = plt.subplots(
        2,
        1,
        figsize=figsize,
        sharex=True,
        gridspec_kw={"height_ratios": [1.35, 3.65], "hspace": 0.05},
    )

    draw_overlay(ax_upper, cycle_buckets, front, alpha, overlap_spread)
    draw_overlay(ax_lower, cycle_buckets, front, alpha, overlap_spread)
    annotate_peak_comparison_broken(fig, ax_upper, ax_lower, datasets, y_break)

    ax_upper.set_ylim(y_break["upper_bottom"], y_break["upper_top"])
    ax_lower.set_ylim(0, y_break["lower_top"])

    ax_upper.set_title(title)

    ax_upper.spines["bottom"].set_visible(False)
    ax_lower.spines["top"].set_visible(False)
    ax_upper.tick_params(labelbottom=False)

    ax_upper.axhline(y_break["upper_bottom"], color="#8d8d8d", linewidth=0.6, alpha=0.35)
    ax_lower.axhline(0, color="#8d8d8d", linewidth=0.8, alpha=0.8)

    finalize_axes(ax_upper)
    finalize_axes(ax_lower)
    ax_upper.legend(handles=build_legend_handles(datasets), loc="upper right", frameon=True)

    draw_break_marks(ax_upper, ax_lower)
    fig.supylabel(column, x=0.04)
    fig.supxlabel("cycle", y=0.04)
    fig.text(
        0.075,
        0.905,
        format_break_annotation(y_break),
        fontsize=9.5,
        color="#5b5b5b",
        ha="left",
        va="top",
    )

    fig.subplots_adjust(left=0.1, right=0.985, top=0.88, bottom=0.14, hspace=0.05)
    fig.savefig(output_path, dpi=dpi)
    plt.close(fig)


def plot_overlay(datasets, column, output_path, title, front, figsize, dpi, alpha, y_break_mode, overlap_spread):
    configure_plot_style()
    base_palette = [
        "#0057b8",
        "#d8b35a",
        "#2f855a",
        "#b24c63",
        "#6b46c1",
        "#00838f",
        "#8c564b",
        "#4c6a92",
    ]
    if len(datasets) <= len(base_palette):
        colors = base_palette[: len(datasets)]
    else:
        colors = base_palette + list(
            sns.color_palette("colorblind", n_colors=len(datasets) - len(base_palette))
        )
    cycle_buckets = build_cycle_buckets(datasets, colors)
    y_break = detect_y_break(datasets) if y_break_mode == "auto" else None

    if y_break is not None:
        plot_broken_axis(
            datasets=datasets,
            cycle_buckets=cycle_buckets,
            column=column,
            output_path=output_path,
            title=title,
            front=front,
            figsize=figsize,
            dpi=dpi,
            alpha=alpha,
            overlap_spread=overlap_spread,
            y_break=y_break,
        )
        return y_break

    fig, ax = plt.subplots(figsize=figsize)
    plot_single_axis(ax, cycle_buckets, datasets, column, title, front, alpha, overlap_spread)
    fig.tight_layout()
    fig.savefig(output_path, dpi=dpi)
    plt.close(fig)
    return None


def main():
    args = parse_args()

    labels = validate_labels(args.csvs, args.labels)
    datasets = [
        load_series(csv_path, args.column, label)
        for csv_path, label in zip(args.csvs, labels)
    ]
    datasets = sort_datasets(datasets)

    output_path = Path(args.output).resolve() if args.output else default_output_path(args.csvs[0], args.column)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    title = args.title or f"Overlay pulse view: {args.column}"
    y_break = plot_overlay(
        datasets=datasets,
        column=args.column,
        output_path=output_path,
        title=title,
        front=args.front,
        figsize=tuple(args.figsize),
        dpi=args.dpi,
        alpha=args.alpha,
        y_break_mode=args.y_break,
        overlap_spread=args.overlap_spread,
    )

    print(f"Saved plot: {output_path}")
    if y_break is not None:
        print(f"Applied y-axis break: {y_break['lower_top']} -> {y_break['upper_bottom']}")
    if not cycles_match(datasets):
        print("Warning: input CSV files do not share identical cycle sequences; plotted on the union of cycles.")
    for dataset in datasets:
        print(
            f"{dataset['label']}: peak={dataset['peak']}, "
            f"sum={dataset['total']}, cycles={dataset['cycle_count']}, "
            f"nonzero_points={dataset['point_count']}, source={dataset['path']}"
        )


if __name__ == "__main__":
    sys.exit(main())
