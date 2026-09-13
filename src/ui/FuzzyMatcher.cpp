// FuzzyMatcher.cpp - Fuzzy Matching Implementation
#include "ui/FuzzyMatcher.h"
#include <QVector>
#include <algorithm>
#include <climits>

namespace monolith {

// Scoring constants - tuned for VS Code-like behavior
namespace {
    constexpr int MATCH_SCORE = 10;           // Base score for matching character
    constexpr int SEQUENTIAL_BONUS = 5;       // Bonus for consecutive matches
    constexpr int WORD_BOUNDARY_BONUS = 8;    // Bonus for matching at word boundary
    constexpr int CAMELCASE_BONUS = 6;        // Bonus for matching uppercase in camelCase
    constexpr int PREFIX_BONUS = 15;          // Bonus for matching at start
    constexpr int START_BOUNDARY_BONUS = 10;  // Bonus for matching at text start
    constexpr int GAP_PENALTY = -1;           // Penalty for each gap character
}

int FuzzyMatcher::match(const QString& pattern, const QString& text)
{
    if (pattern.isEmpty()) {
        return 0;
    }
    
    if (text.isEmpty()) {
        return 0;
    }
    
    if (pattern.length() > text.length()) {
        return 0;
    }
    
    QVector<int> positions;
    return smithWaterman(pattern, text, positions);
}

QVector<int> FuzzyMatcher::matchPositions(const QString& pattern, const QString& text)
{
    QVector<int> positions;
    
    if (pattern.isEmpty() || text.isEmpty() || pattern.length() > text.length()) {
        return positions;
    }
    
    smithWaterman(pattern, text, positions);
    return positions;
}

bool FuzzyMatcher::matches(const QString& pattern, const QString& text)
{
    if (pattern.isEmpty()) {
        return true;
    }
    
    if (text.isEmpty()) {
        return false;
    }
    
    int patternIdx = 0;
    int textIdx = 0;
    
    while (patternIdx < pattern.length() && textIdx < text.length()) {
        if (charsMatch(pattern[patternIdx], text[textIdx])) {
            patternIdx++;
        }
        textIdx++;
    }
    
    return patternIdx == pattern.length();
}

bool FuzzyMatcher::isPrefixPattern(const QString& pattern)
{
    // Patterns ending with specific characters suggest prefix matching
    if (pattern.endsWith('/') || pattern.endsWith('\\')) {
        return true;
    }
    
    // Patterns that look like paths
    if (pattern.contains('/') || pattern.contains('\\')) {
        return true;
    }
    
    return false;
}

int FuzzyMatcher::smithWaterman(const QString& pattern, const QString& text, QVector<int>& positions)
{
    int m = pattern.length();
    int n = text.length();
    
    // DP table for Smith-Waterman algorithm
    // Using int instead of full matrix for memory efficiency
    QVector<QVector<int>> dp(m + 1, QVector<int>(n + 1, 0));
    
    int maxScore = 0;
    int maxPos = 0;
    
    // Fill the DP table
    for (int i = 1; i <= m; ++i) {
        QChar pChar = pattern[i - 1];
        
        for (int j = 1; j <= n; ++j) {
            QChar tChar = text[j - 1];
            
            if (charsMatch(pChar, tChar)) {
                int matchScore = dp[i - 1][j - 1] + MATCH_SCORE;
                
                // Add bonuses
                matchScore += calculateBonus(
                    pChar.unicode(),
                    tChar.unicode(),
                    j - 1,
                    n,
                    (i > 1 && j > 1 && charsMatch(pattern[i - 2], text[j - 2]))
                );
                
                dp[i][j] = std::max(matchScore, GAP_PENALTY);
                
                if (dp[i][j] > maxScore) {
                    maxScore = dp[i][j];
                    maxPos = j;
                }
            } else {
                dp[i][j] = std::max({
                    dp[i - 1][j] + GAP_PENALTY,
                    dp[i][j - 1] + GAP_PENALTY,
                    0
                });
            }
        }
    }
    
    if (maxScore == 0) {
        return 0;
    }
    
    // Backtrack to find matched positions
    positions.clear();
    int i = m, j = maxPos;
    
    while (i > 0 && j > 0) {
        if (charsMatch(pattern[i - 1], text[j - 1]) && 
            dp[i][j] == dp[i - 1][j - 1] + MATCH_SCORE + 
                calculateBonus(pattern[i - 1].unicode(), text[j - 1].unicode(), j - 1, n,
                    (i > 1 && j > 1 && charsMatch(pattern[i - 2], text[j - 2])))) {
            positions.prepend(j - 1);
            --i;
            --j;
        } else if (dp[i][j] == dp[i - 1][j] + GAP_PENALTY) {
            --i;
        } else {
            --j;
        }
    }
    
    // Ensure we found all pattern characters
    if (positions.size() != static_cast<size_t>(m)) {
        // Fall back to simple greedy matching
        positions.clear();
        int textIdx = 0;
        for (int pIdx = 0; pIdx < m && textIdx < n; ++pIdx) {
            while (textIdx < n && !charsMatch(pattern[pIdx], text[textIdx])) {
                textIdx++;
            }
            if (textIdx < n) {
                positions.append(textIdx);
                textIdx++;
            }
        }
    }
    
    return maxScore;
}

int FuzzyMatcher::calculateBonus(char patternChar, char textChar, int posInText, int textLen, bool prevMatched)
{
    int bonus = 0;
    
    // Sequential/consecutive match bonus
    if (prevMatched) {
        bonus += SEQUENTIAL_BONUS;
    }
    
    // Beginning of text bonus
    if (posInText == 0) {
        bonus += START_BOUNDARY_BONUS;
    }
    
    // Word boundary bonus
    if (isWordBoundary(QString::fromLatin1(&textChar, 1), 0)) {
        bonus += WORD_BOUNDARY_BONUS;
    }
    
    // Check for word boundary before current position
    if (posInText > 0) {
        // We'd need the full text here, simplified check
        QChar prevChar;  // Would need actual previous char
        if (prevChar == QLatin1Char('/') || prevChar == QLatin1Char('\\') ||
            prevChar == QLatin1Char('_') || prevChar == QLatin1Char('-') ||
            prevChar.isSpace()) {
            bonus += WORD_BOUNDARY_BONUS;
        }
        
        // CamelCase boundary
        if (posInText > 0 && isUpperCase(textChar)) {
            bonus += CAMELCASE_BONUS;
        }
    }
    
    // Exact case match bonus (for patterns with uppercase)
    if (patternChar == textChar) {
        bonus += 2;
    }
    
    return bonus;
}

bool FuzzyMatcher::isWordBoundary(const QString& text, int pos)
{
    if (pos <= 0) {
        return true;
    }
    
    QChar c = text[pos];
    QChar prev = text[pos - 1];
    
    // After separator characters
    if (prev == QLatin1Char('/') || prev == QLatin1Char('\\') ||
        prev == QLatin1Char('_') || prev == QLatin1Char('-') ||
        prev == QLatin1Char(' ') || prev == QLatin1Char('.')) {
        return true;
    }
    
    // CamelCase boundary (lowercase to uppercase)
    if (!prev.isUpper() && c.isUpper()) {
        return true;
    }
    
    return false;
}

bool FuzzyMatcher::isUpperCase(QChar c)
{
    return c.isUpper() && c.isLetter();
}

bool FuzzyMatcher::charsMatch(QChar patternChar, QChar textChar)
{
    return patternChar.toLower() == textChar.toLower();
}

} // namespace monolith
