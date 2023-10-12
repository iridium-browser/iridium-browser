#[derive(Eq, Clone)]
pub enum Bool {
    True,
    False,
    /// can be any number in `0..32`, anything else will cause panics or wrong results
    Term(u8),
    /// needs to contain at least two elements
    And(Vec<Bool>),
    /// needs to contain at least two elements
    Or(Vec<Bool>),
    Not(Box<Bool>),
}

#[cfg(feature="quickcheck")]
extern crate quickcheck;

#[cfg(feature="quickcheck")]
impl quickcheck::Arbitrary for Bool {
    fn arbitrary<G: quickcheck::Gen>(g: &mut G) -> Self {
        let mut terms = 0;
        arbitrary_bool(g, 10, &mut terms)
    }
    fn shrink(&self) -> Box<Iterator<Item=Self>> {
        match *self {
            Bool::And(ref v) => Box::new(v.shrink().filter(|v| v.len() > 2).map(Bool::And)),
            Bool::Or(ref v) => Box::new(v.shrink().filter(|v| v.len() > 2).map(Bool::Or)),
            Bool::Not(ref inner) => Box::new(inner.shrink().map(|b| Bool::Not(Box::new(b)))),
            _ => quickcheck::empty_shrinker(),
        }
    }
}

#[cfg(feature="quickcheck")]
fn arbitrary_bool<G: quickcheck::Gen>(g: &mut G, depth: usize, terms: &mut u8) -> Bool {
    if depth == 0 {
        match g.gen_range(0, 3) {
            0 => Bool::True,
            1 => Bool::False,
            2 => arbitrary_term(g, terms),
            _ => unreachable!(),
        }
    } else {
        match g.gen_range(0, 6) {
            0 => Bool::True,
            1 => Bool::False,
            2 => arbitrary_term(g, terms),
            3 => Bool::And(arbitrary_vec(g, depth - 1, terms)),
            4 => Bool::Or(arbitrary_vec(g, depth - 1, terms)),
            5 => Bool::Not(Box::new(arbitrary_bool(g, depth - 1, terms))),
            _ => unreachable!(),
        }
    }
}

#[cfg(feature="quickcheck")]
fn arbitrary_term<G: quickcheck::Gen>(g: &mut G, terms: &mut u8) -> Bool {
    if *terms == 0 {
        Bool::Term(*terms)
    } else if *terms < 32 && g.gen_weighted_bool(3) {
        *terms += 1;
        // every term needs to show up at least once
        Bool::Term(*terms - 1)
    } else {
        Bool::Term(g.gen_range(0, *terms))
    }
}

#[cfg(feature="quickcheck")]
fn arbitrary_vec<G: quickcheck::Gen>(g: &mut G, depth: usize, terms: &mut u8) -> Vec<Bool> {
    (0..g.gen_range(2, 20)).map(|_| arbitrary_bool(g, depth, terms)).collect()
}

impl PartialEq for Bool {
    fn eq(&self, other: &Self) -> bool {
        use self::Bool::*;
        match (self, other) {
            (&True, &True) |
            (&False, &False) => true,
            (&Term(a), &Term(b)) => a == b,
            (&Not(ref a), &Not(ref b)) => a == b,
            (&And(ref a), &And(ref b)) |
            (&Or(ref a), &Or(ref b)) => {
                if a.len() != b.len() {
                    return false;
                }
                for a in a {
                    if !b.iter().any(|b| b == a) {
                        return false;
                    }
                }
                true
            },
            _ => false,
        }
    }
}

impl std::ops::Not for Bool {
    type Output = Bool;
    fn not(self) -> Bool {
        use self::Bool::*;
        match self {
            True => False,
            False => True,
            t @ Term(_) => Not(Box::new(t)),
            And(mut v) => {
                for el in &mut v {
                    *el = !std::mem::replace(el, False);
                }
                Or(v)
            },
            Or(mut v) => {
                for el in &mut v {
                    *el = !std::mem::replace(el, False);
                }
                And(v)
            },
            Not(inner) => *inner,
        }
    }
}

impl std::ops::BitAnd for Bool {
    type Output = Self;
    fn bitand(self, rhs: Bool) -> Bool {
        use self::Bool::*;
        match (self, rhs) {
            (And(mut v), And(rhs)) => {
                v.extend(rhs);
                And(v)
            },
            (False, _) |
            (_, False) => False,
            (b, True) |
            (True, b) => b,
            (other, And(mut v)) |
            (And(mut v), other) => {
                v.push(other);
                And(v)
            },
            (a, b) => And(vec![a, b]),
        }
    }
}

impl std::ops::BitOr for Bool {
    type Output = Self;
    fn bitor(self, rhs: Bool) -> Bool {
        use self::Bool::*;
        match (self, rhs) {
            (Or(mut v), Or(rhs)) => {
                v.extend(rhs);
                Or(v)
            },
            (False, b) |
            (b, False) => b,
            (_, True) |
            (True, _) => True,
            (other, Or(mut v)) |
            (Or(mut v), other) => {
                v.push(other);
                Or(v)
            },
            (a, b) => Or(vec![a, b]),
        }
    }
}

impl Bool {
    pub fn terms(&self) -> u32 {
        use self::Bool::*;
        match *self {
            Term(u) => 1 << u,
            Or(ref a) |
            And(ref a) => a.iter().fold(0, |state, item| { state | item.terms() }),
            Not(ref a) => a.terms(),
            True | False => 0,
        }
    }

    pub fn eval(&self, terms: u32) -> bool {
        use self::Bool::*;
        match *self {
            True => true,
            False => false,
            Term(i) => (terms & (1 << i)) != 0,
            And(ref a) => a.iter().all(|item| item.eval(terms)),
            Or(ref a) => a.iter().any(|item| item.eval(terms)),
            Not(ref a) => !a.eval(terms),
        }
    }

    pub fn minterms(&self) -> Vec<Term> {
        let terms = self.terms();
        let number_of_terms = terms.count_ones();
        assert!((0..number_of_terms).all(|i| (terms & (1 << i)) != 0), "non-continuous naming scheme");
        (0..(1 << number_of_terms)).filter(|&i| self.eval(i)).map(Term::new).collect()
    }

    pub fn simplify(&self) -> Vec<Bool> {
        let minterms = self.minterms();
        if minterms.is_empty() {
            return vec![Bool::False];
        }
        let variables = self.terms().count_ones();
        if variables == 0 {
            return vec![Bool::True];
        }
        let essentials = essential_minterms(minterms);
        let expr = essentials.prime_implicant_expr();
        let simple = simplify_prime_implicant_expr(expr);
        let shortest = simple.iter().map(Vec::len).min().unwrap();
        simple.into_iter()
              .filter(|v| v.len() == shortest)
              .map(|v| {
                  let mut v = v.into_iter()
                               .map(|i| essentials.essentials[i as usize]
                                                  .to_bool_expr(variables));
                  if v.len() == 1 {
                      v.next().unwrap()
                  } else {
                      Bool::Or(v.collect())
                  }
              }).collect()
    }
}

impl std::fmt::Debug for Bool {
    fn fmt(&self, fmt: &mut std::fmt::Formatter) -> std::fmt::Result {
        use self::Bool::*;
        match *self {
            True => write!(fmt, "T"),
            False => write!(fmt, "F"),
            Term(i) if i > 32 => write!(fmt, "<bad term id {}>", i),
            Term(i) => write!(fmt, "{}", "abcdefghijklmnopqrstuvwxyzαβγδεζη".chars().nth(i as usize).unwrap()),
            Not(ref a) => match **a {
                And(_) | Or(_) => write!(fmt, "({:?})'", a),
                _ => write!(fmt, "{:?}'", a),
            },
            And(ref a) => {
                for a in a {
                    match *a {
                        And(_) | Or(_) => try!(write!(fmt, "({:?})", a)),
                        _ => try!(write!(fmt, "{:?}", a)),
                    }
                }
                Ok(())
            },
            Or(ref a) => {
                try!(write!(fmt, "{:?}", a[0]));
                for a in &a[1..] {
                    match *a {
                        Or(_) => try!(write!(fmt, " + ({:?})", a)),
                        _ => try!(write!(fmt, " + {:?}", a)),
                    }
                }
                Ok(())
            }
        }
    }
}

#[derive(Debug)]
pub struct Essentials {
    pub minterms: Vec<Term>,
    pub essentials: Vec<Term>,
}

pub fn simplify_prime_implicant_expr(mut e: Vec<Vec<Vec<u32>>>) -> Vec<Vec<u32>> {
    loop {
        let a = e.pop().unwrap();
        if let Some(b) = e.pop() {
            let distributed = distribute(&a, &b);
            let simplified = simplify(distributed);
            e.push(simplified);
        } else {
            return a;
        }
    }
}

// AA     -> A
// A + AB -> A
// A + A  -> A
fn simplify(mut e: Vec<Vec<u32>>) -> Vec<Vec<u32>> {
    for e in &mut e {
        e.sort();
        e.dedup();
    }
    e.sort();
    e.dedup();
    let mut del = Vec::new();
    for (i, a) in e.iter().enumerate() {
        for (j, b) in e[i..].iter().enumerate() {
            if a.len() < b.len() {
                // A + AB -> delete AB
                if a.iter().all(|x| b.iter().any(|y| y == x)) {
                    del.push(j + i);
                }
            } else if b.len() < a.len() && b.iter().all(|x| a.iter().any(|y| y == x)) {
                // AB + A -> delete AB
                del.push(i);
            }
        }
    }
    del.sort();
    del.dedup();
    for del in del.into_iter().rev() {
        e.swap_remove(del);
    }
    e
}

// (AB + CD)(EF + GH) -> ABEF + ABGH + CDEF + CDGH
fn distribute(l: &[Vec<u32>], r: &[Vec<u32>]) -> Vec<Vec<u32>> {
    let mut ret = Vec::new();
    assert!(!l.is_empty());
    assert!(!r.is_empty());
    for l in l {
        for r in r {
            ret.push(l.iter().chain(r).cloned().collect());
        }
    }
    ret
}


impl Essentials {
    pub fn prime_implicant_expr(&self) -> Vec<Vec<Vec<u32>>> {
        let mut v = Vec::new();
        for t in &self.minterms {
            let mut w = Vec::new();
            for (i, e) in self.essentials.iter().enumerate() {
                if e.contains(t) {
                    assert_eq!(i as u32 as usize, i);
                    w.push(vec![i as u32]);
                }
            }
            v.push(w);
        }
        v
    }
}

#[derive(Clone, Eq, Ord)]
pub struct Term {
    dontcare: u32,
    term: u32,
}

#[cfg(feature="quickcheck")]
impl quickcheck::Arbitrary for Term {
    fn arbitrary<G: quickcheck::Gen>(g: &mut G) -> Self {
        Term {
            dontcare: u32::arbitrary(g),
            term: u32::arbitrary(g),
        }
    }
}

impl std::cmp::PartialOrd for Term {
    fn partial_cmp(&self, rhs: &Self) -> Option<std::cmp::Ordering> {
        use std::cmp::Ordering::*;
        match self.dontcare.partial_cmp(&rhs.dontcare) {
            Some(Equal) => {},
            other => return other,
        }
        let l = self.term & !self.dontcare;
        let r = rhs.term & !rhs.dontcare;
        l.partial_cmp(&r)
    }
}

impl std::fmt::Debug for Term {
    fn fmt(&self, fmt: &mut std::fmt::Formatter) -> std::fmt::Result {
        for i in (0..32).rev() {
            if (self.dontcare & (1 << i)) == 0 {
                if (self.term & (1 << i)) == 0 {
                    try!(write!(fmt, "0"));
                } else {
                    try!(write!(fmt, "1"));
                }
            } else {
                try!(write!(fmt, "-"));
            }
        }
        Ok(())
    }
}

impl std::cmp::PartialEq for Term {
    fn eq(&self, other: &Self) -> bool {
        (self.dontcare == other.dontcare) && ((self.term & !self.dontcare) == (other.term & !other.dontcare))
    }
}

#[derive(Debug, Eq, PartialEq)]
pub enum TermFromStrError {
    Only32TermsSupported,
    UnsupportedCharacter(char),
}

impl std::str::FromStr for Term {
    type Err = TermFromStrError;
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        if s.len() > 32 {
            return Err(TermFromStrError::Only32TermsSupported);
        }
        let mut term = Term::new(0);
        for (i, c) in s.chars().rev().enumerate() {
            match c {
                '-' => term.dontcare |= 1 << i,
                '1' => term.term |= 1 << i,
                '0' => {},
                c => return Err(TermFromStrError::UnsupportedCharacter(c)),
            }
        }
        Ok(term)
    }
}

impl Term {
    pub fn new(i: u32) -> Self {
        Term {
            dontcare: 0,
            term: i,
        }
    }

    pub fn with_dontcare(term: u32, dontcare: u32) -> Self {
        Term {
            dontcare: dontcare,
            term: term,
        }
    }

    pub fn combine(&self, other: &Term) -> Option<Term> {
        let dc = self.dontcare ^ other.dontcare;
        let term = self.term ^ other.term;
        let dc_mask = self.dontcare | other.dontcare;
        match (dc.count_ones(), (!dc_mask & term).count_ones()) {
            (0, 1) |
            (1, 0) => Some(Term {
                dontcare: dc_mask | term,
                term: self.term,
            }),
            _ => None,
        }
    }

    pub fn contains(&self, other: &Self) -> bool {
        ((self.dontcare | other.dontcare) == self.dontcare) &&
        (((self.term ^ other.term) & !self.dontcare) == 0)
    }

    pub fn to_bool_expr(&self, n_variables: u32) -> Bool {
        assert!(self.dontcare < (1 << n_variables));
        assert!(self.term < (1 << n_variables));
        let mut v = Vec::new();
        for bit in 0..n_variables {
            if (self.dontcare & (1 << bit)) == 0 {
                if (self.term & (1 << bit)) == 0 {
                    v.push(Bool::Not(Box::new(Bool::Term(bit as u8))))
                } else {
                    v.push(Bool::Term(bit as u8))
                }
            }
        }
        match v.len() {
            0 => Bool::True,
            1 => v.into_iter().next().unwrap(),
            _ => Bool::And(v),
        }
    }
}

pub fn essential_minterms(mut minterms: Vec<Term>) -> Essentials {
    minterms.sort();
    let minterms = minterms;
    let mut terms = minterms.clone();
    let mut essentials: Vec<Term> = Vec::new();
    while !terms.is_empty() {
        let old = std::mem::replace(&mut terms, Vec::new());
        let mut combined_terms = std::collections::BTreeSet::new();
        for (i, term) in old.iter().enumerate() {
            for (other_i, other) in old[i..].iter().enumerate() {
                if let Some(new_term) = term.combine(other) {
                    terms.push(new_term);
                    combined_terms.insert(other_i + i);
                    combined_terms.insert(i);
                }
            }
            if !combined_terms.contains(&i) {
                essentials.push(term.clone());
            }
        }
        terms.sort();
        terms.dedup();
    }
    Essentials {
        minterms: minterms,
        essentials: essentials,
    }
}
