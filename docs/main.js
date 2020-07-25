const container = document.getElementById("container");
for (let i in plotData) {
    const data = plotData[i];
    const graphDiv = document.createElement("div");
    container.append(graphDiv);
    Highcharts.chart(graphDiv, {
        chart: {
            zoomType: "xy",
            width: 1000
        },
        title: {
            text: plotData[i].name
        },
        xAxis: {
            type: data.isLog ? "logarithmic" : undefined,
            title: {
                text: data.titleX || undefined
            }
        },
        yAxis: {
            title: {
                text: data.titleY || undefined
            },
            tickPixelInterval: 25,
            floor: -50,
            ceiling: 50
        },
        plotOptions: {
            series: {
                marker: {
                    enabled: false
                }
            },
            line: {
                animation: false
            }
        },
        legend: {
            align: "right",
            verticalAlign: "top",
            layout: "vertical",
            x: 0,
            y: 100
        },
        series: data.series,
        credits: {
            enabled: false
        }
    });
}