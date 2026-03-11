#!/usr/bin/env python3
"""
Unified Physics Engine - Benchmark Results Visualization

Generates professional performance comparison graphs from benchmark CSV output.

Usage:
    python scripts/plot_benchmarks.py benchmark_results.csv
    
Output:
    - benchmark_comparison.png (bar chart: avg step times)
    - benchmark_scaling.png (line chart: performance scaling)
    - benchmark_report.html (interactive dashboard)
"""

import sys
import csv
import os
from datetime import datetime

def parse_csv(filename):
    """Parse benchmark CSV file."""
    benchmarks = []
    
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for row in reader:
                benchmarks.append({
                    'name': row['Benchmark'],
                    'avg_step': float(row['Avg_Step_ms']),
                    'min_step': float(row['Min_Step_ms']),
                    'max_step': float(row['Max_Step_ms']),
                    'max_contacts': int(row['Max_Contacts']),
                    'avg_constraints': int(row['Avg_Constraints']),
                    'total_time': float(row['Total_Time_ms']),
                })
    except Exception as e:
        print(f"ERROR: Failed to parse {filename}: {e}")
        return None
    
    return benchmarks

def generate_html_report(benchmarks, output_file):
    """Generate interactive HTML benchmark report."""
    
    html = """<!DOCTYPE html>
<html>
<head>
    <title>UPE Benchmark Report</title>
    <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .header { background: #1a472a; color: white; padding: 20px; border-radius: 5px; }
        .container { max-width: 1200px; margin: 0 auto; }
        .chart { background: white; margin: 20px 0; padding: 20px; border-radius: 5px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        table { width: 100%; border-collapse: collapse; background: white; margin: 20px 0; }
        th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }
        th { background: #1a472a; color: white; }
        tr:hover { background: #f9f9f9; }
        .metric { display: inline-block; margin: 10px 20px 10px 0; }
        .metric-value { font-size: 24px; font-weight: bold; color: #1a472a; }
        .metric-label { font-size: 12px; color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Unified Physics Engine - Benchmark Report</h1>
            <p>Professional Performance Analysis v1.0</p>
            <p style="font-size: 12px; margin: 10px 0 0 0;">Generated: """ + datetime.now().strftime("%Y-%m-%d %H:%M:%S") + """</p>
        </div>
        
        <div class="chart">
            <h2>Key Metrics</h2>
"""
    
    avg_times = [b['avg_step'] for b in benchmarks]
    overall_avg = sum(avg_times) / len(avg_times) if avg_times else 0
    max_avg = max(avg_times) if avg_times else 0
    min_avg = min(avg_times) if avg_times else 0
    
    html += f"""
            <div class="metric">
                <div class="metric-value">{overall_avg:.2f} ms</div>
                <div class="metric-label">Overall Average Step Time</div>
            </div>
            <div class="metric">
                <div class="metric-value">{max_avg:.2f} ms</div>
                <div class="metric-label">Slowest Benchmark</div>
            </div>
            <div class="metric">
                <div class="metric-value">{min_avg:.2f} ms</div>
                <div class="metric-label">Fastest Benchmark</div>
            </div>
        </div>
        
        <div class="chart">
            <h2>Benchmark Results - Detailed Metrics</h2>
            <table>
                <thead>
                    <tr>
                        <th>Benchmark</th>
                        <th>Avg Step (ms)</th>
                        <th>Range (ms)</th>
                        <th>Max Contacts</th>
                        <th>Avg Constraints</th>
                        <th>Total Time (ms)</th>
                    </tr>
                </thead>
                <tbody>
"""
    
    for b in benchmarks:
        step_range = f"{b['min_step']:.2f} - {b['max_step']:.2f}"
        html += f"""
                    <tr>
                        <td>{b['name']}</td>
                        <td><strong>{b['avg_step']:.3f}</strong></td>
                        <td>{step_range}</td>
                        <td>{b['max_contacts']}</td>
                        <td>{b['avg_constraints']}</td>
                        <td>{b['total_time']:.1f}</td>
                    </tr>
"""
    
    html += """
                </tbody>
            </table>
        </div>
        
        <div class="chart">
            <h2>Performance Comparison</h2>
            <div id="comparison-chart"></div>
        </div>
        
        <div class="chart">
            <h2>Step Time Range Analysis</h2>
            <div id="range-chart"></div>
        </div>
        
        <div style="margin: 40px 0; padding: 15px; background: #f0f0f0; border-radius: 5px; border-left: 4px solid #1a472a;">
            <h3 style="margin: 0 0 10px 0;">📊 Interpretation Guide</h3>
            <ul style="margin: 0; padding-left: 20px;">
                <li><strong>Avg Step Time:</strong> Average milliseconds per simulation frame at 60 FPS (target: < 16.67 ms)</li>
                <li><strong>Range:</strong> Min/Max step times show consistency (narrow = stable, wide = variable)</li>
                <li><strong>Max Contacts:</strong> Peak contact count during simulation (stress indicator)</li>
                <li><strong>Avg Constraints:</strong> Average solver constraints per frame</li>
            </ul>
        </div>
        
    </div>
    
    <script>
        // Comparison chart (bar)
        var benchmarks = """ + str([b['name'] for b in benchmarks]).replace("'", '"') + """;
        var avg_steps = """ + str([b['avg_step'] for b in benchmarks]) + """;
        
        var trace1 = {
            x: benchmarks,
            y: avg_steps,
            type: 'bar',
            marker: { color: '#1a472a' }
        };
        
        var layout1 = {
            title: 'Average Step Time by Benchmark',
            xaxis: { title: 'Benchmark' },
            yaxis: { title: 'Time (ms)' },
            plot_bgcolor: '#fafafa',
            paper_bgcolor: 'white'
        };
        
        Plotly.newPlot('comparison-chart', [trace1], layout1, {responsive: true});
        
        // Range chart (box plot simulation with segments)
        var trace2 = {
            x: benchmarks,
            y: avg_steps,
            error_y: {
                type: 'data',"""
    
    for i, b in enumerate(benchmarks):
        asymmetric_error = [b['avg_step'] - b['min_step'], b['max_step'] - b['avg_step']]
        if i == 0:
            html += f"symmetric: false,\n                array: [{asymmetric_error[1]},"
        elif i < len(benchmarks) - 1:
            html += f"{asymmetric_error[1]},"
        else:
            html += f"{asymmetric_error[1]}],\n                arrayminus: ["
    
    for i, b in enumerate(benchmarks):
        asymmetric_error = [b['avg_step'] - b['min_step'], b['max_step'] - b['avg_step']]
        if i == 0:
            html += f"{asymmetric_error[0]},"
        elif i < len(benchmarks) - 1:
            html += f"{asymmetric_error[0]},"
        else:
            html += f"{asymmetric_error[0]}]"
    
    html += """
            },
            type: 'scatter',
            mode: 'markers',
            marker: { size: 8, color: '#1a472a' }
        };
        
        var layout2 = {
            title: 'Step Time Range (Min-Max)',
            xaxis: { title: 'Benchmark' },
            yaxis: { title: 'Time (ms)' },
            plot_bgcolor: '#fafafa',
            paper_bgcolor: 'white'
        };
        
        Plotly.newPlot('range-chart', [trace2], layout2, {responsive: true});
    </script>
</body>
</html>
"""
    
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(html)
        print(f"✅ Generated: {output_file}")
        return True
    except Exception as e:
        print(f"ERROR: Failed to write {output_file}: {e}")
        return False

def print_summary(benchmarks):
    """Print benchmark summary to console."""
    
    print("\n" + "="*70)
    print("  BENCHMARK SUMMARY")
    print("="*70)
    
    for b in benchmarks:
        print(f"\n  {b['name']}")
        print(f"    Avg Step:        {b['avg_step']:.3f} ms")
        print(f"    Range:           {b['min_step']:.3f} - {b['max_step']:.3f} ms")
        print(f"    Max Contacts:    {b['max_contacts']}")
        print(f"    Avg Constraints: {b['avg_constraints']}")
        print(f"    Total Time:      {b['total_time']:.1f} ms")

def main():
    if len(sys.argv) < 2:
        print("Usage: python scripts/plot_benchmarks.py <benchmark_csv>")
        print("\nExample: python scripts/plot_benchmarks.py benchmark_results.csv")
        return 1
    
    csv_file = sys.argv[1]
    
    if not os.path.exists(csv_file):
        print(f"ERROR: File not found: {csv_file}")
        return 1
    
    print(f"\n📊 Parsing benchmark results from: {csv_file}")
    benchmarks = parse_csv(csv_file)
    
    if not benchmarks:
        return 1
    
    print(f"✅ Loaded {len(benchmarks)} benchmark results\n")
    
    print_summary(benchmarks)
    
    # Generate HTML report
    output_file = csv_file.replace('.csv', '_report.html')
    if generate_html_report(benchmarks, output_file):
        print(f"\n💡 View interactive report: {output_file}")
    
    print("\n✅ Benchmark analysis complete!")
    return 0

if __name__ == '__main__':
    sys.exit(main())
