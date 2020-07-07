import "owp.core/string/capitalizeFirst";

export default {
    clean: (str) => {
        const parts = str.split("_");
        const cleanParts = parts.map(p => p.toLowerCase().capitalizeFirst());
        return cleanParts.join(" ");
    }
}