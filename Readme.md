# CXXMetris: A Fast, Lightweight, In-Thread C++ Metrics Library

# Idea

四个模块：

名字到编号；测量器；测量器的管理；报告器

## 名字到编号

考虑尽量把内容放到编译时，运行时无字符串处理。
现在能实现的：通过编译时模板，用字符串字面量作模板参数，结合static变量，将所有字符串转换为static的ID。
限制：无法给每个Metrics对象都拥有一套该系统，只能全局的将字符串设置为0~N的密集ID。

考虑：Metrics也在编译时定义，如`Metrics<"first">`和`Metrics<"second">`，这样每个Metrics就能拥有独立的一套系统。
缺点：太蠢了。

考虑：每个对象都用一个大小为N的数据或`vector`记录所有字符串，并将其分配ID。
缺点：略大的空间占用，特别是字符串很多时，而且比较像Hash了。

考虑：编译期（constexpr）的Hash。
优点：优雅且兼容运行时。
缺点：冲突，开销会略大一些。

## 测量器

测量器通常考虑和数据分离？可能的想法：

* 测量器类似`std::hash`那样，使用一个空的对象来实现。
* 测量器中记录`MetricsEntry`的指针或引用，同时`Entry`也记录有测量器的指针（可向下转型，测量器可继承，包含Entry的逻辑在基类中实现）。
* 测量器纯`static`，只作为一种仪表，记录当前的某种状态。MetricsEntry本身只记录数据。通过`MetricsEntry`本身调用测量器获取数据。
  如，`m.Record<Timer>()`。
* PMR分配
