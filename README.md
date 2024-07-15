# Run Command
- `$ sh compile-run.sh`

# Credit
 - http://dmitrysoshnikov.com/

# Example Output
```
 ; ModuleID = './out.ll'
source_filename = "JovianVM"
target triple = "x86_64-pc-linux-gnu"

%Point_vTable = type { i32 (%Point*)*, i32 (%Point*, i32, i32)* }
%Point = type { %Point_vTable*, i32, i32 }
%Point3D_vTable = type { i32 (%Point3D*)*, i32 (%Point3D*, i32, i32, i32)* }
%Point3D = type { %Point3D_vTable*, i32, i32, i32 }

@VERSION = local_unnamed_addr global i32 42, align 4
@Point_vTable = global %Point_vTable { i32 (%Point*)* @Point_calc, i32 (%Point*, i32, i32)* @Point_constructor }, align 4
@Point3D_vTable = global %Point3D_vTable { i32 (%Point3D*)* @Point3D_calc, i32 (%Point3D*, i32, i32, i32)* @Point3D_constructor }, align 4
@0 = private unnamed_addr constant [11 x i8] c"p2.x = %d\0A\00", align 1
@1 = private unnamed_addr constant [11 x i8] c"p2.y = %d\0A\00", align 1
@2 = private unnamed_addr constant [11 x i8] c"p2.z = %d\0A\00", align 1
@3 = private unnamed_addr constant [26 x i8] c"Point3D.calc result = %d\0A\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(i8* nocapture noundef readonly, ...) local_unnamed_addr #0

; Function Attrs: inaccessiblememonly mustprogress nofree nounwind willreturn
declare noalias noundef i8* @malloc(i64 noundef) local_unnamed_addr #1

define i32 @main() local_unnamed_addr {
entry:
  %p1 = tail call dereferenceable_or_null(16) i8* @malloc(i64 16)
  %0 = bitcast i8* %p1 to %Point*
  %1 = getelementptr inbounds %Point, %Point* %0, i64 0, i32 0
  store %Point_vTable* @Point_vTable, %Point_vTable** %1, align 8
  %px.i = getelementptr inbounds %Point, %Point* %0, i64 0, i32 1
  store i32 10, i32* %px.i, align 4
  %py.i = getelementptr inbounds %Point, %Point* %0, i64 0, i32 2
  store i32 20, i32* %py.i, align 4
  %p2 = tail call dereferenceable_or_null(24) i8* @malloc(i64 24)
  %2 = bitcast i8* %p2 to %Point3D*
  %3 = getelementptr inbounds %Point3D, %Point3D* %2, i64 0, i32 0
  store %Point3D_vTable* @Point3D_vTable, %Point3D_vTable** %3, align 8
  %4 = load i32 (%Point*, i32, i32)*, i32 (%Point*, i32, i32)** getelementptr inbounds (%Point_vTable, %Point_vTable* @Point_vTable, i64 0, i32 1), align 8
  %5 = bitcast i8* %p2 to %Point*
  %6 = tail call i32 %4(%Point* %5, i32 100, i32 200)
  %pz.i = getelementptr inbounds %Point3D, %Point3D* %2, i64 0, i32 3
  store i32 300, i32* %pz.i, align 4
  %px = getelementptr inbounds %Point3D, %Point3D* %2, i64 0, i32 1
  %x = load i32, i32* %px, align 4
  %7 = tail call i32 (i8*, ...) @printf(i8* nonnull dereferenceable(1) getelementptr inbounds ([11 x i8], [11 x i8]* @0, i64 0, i64 0), i32 %x)
  %py = getelementptr inbounds %Point3D, %Point3D* %2, i64 0, i32 2
  %y = load i32, i32* %py, align 4
  %8 = tail call i32 (i8*, ...) @printf(i8* nonnull dereferenceable(1) getelementptr inbounds ([11 x i8], [11 x i8]* @1, i64 0, i64 0), i32 %y)
  %z = load i32, i32* %pz.i, align 4
  %9 = tail call i32 (i8*, ...) @printf(i8* nonnull dereferenceable(1) getelementptr inbounds ([11 x i8], [11 x i8]* @2, i64 0, i64 0), i32 %z)
  %vt = load %Point3D_vTable*, %Point3D_vTable** %3, align 8
  %10 = getelementptr inbounds %Point3D_vTable, %Point3D_vTable* %vt, i64 0, i32 0
  %11 = load i32 (%Point3D*)*, i32 (%Point3D*)** %10, align 8
  %12 = tail call i32 %11(%Point3D* %2)
  %13 = tail call i32 (i8*, ...) @printf(i8* nonnull dereferenceable(1) getelementptr inbounds ([26 x i8], [26 x i8]* @3, i64 0, i64 0), i32 %12)
  %14 = load i32 (%Point*)*, i32 (%Point*)** getelementptr inbounds (%Point_vTable, %Point_vTable* @Point_vTable, i64 0, i32 0), align 8
  %15 = tail call i32 %14(%Point* %0)
  %16 = getelementptr inbounds %Point, %Point* %5, i64 0, i32 0
  %vt.i1 = load %Point_vTable*, %Point_vTable** %16, align 8
  %17 = getelementptr inbounds %Point_vTable, %Point_vTable* %vt.i1, i64 0, i32 0
  %18 = load i32 (%Point*)*, i32 (%Point*)** %17, align 8
  %19 = tail call i32 %18(%Point* %5)
  ret i32 0
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn writeonly
define i32 @Point_constructor(%Point* nocapture writeonly %self, i32 %x, i32 returned %y) #2 {
entry:
  %px = getelementptr inbounds %Point, %Point* %self, i64 0, i32 1
  store i32 %x, i32* %px, align 4
  %py = getelementptr inbounds %Point, %Point* %self, i64 0, i32 2
  store i32 %y, i32* %py, align 4
  ret i32 %y
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind readonly willreturn
define i32 @Point_calc(%Point* nocapture readonly %self) #3 {
entry:
  %px = getelementptr inbounds %Point, %Point* %self, i64 0, i32 1
  %x = load i32, i32* %px, align 4
  %py = getelementptr inbounds %Point, %Point* %self, i64 0, i32 2
  %y = load i32, i32* %py, align 4
  %tmpadd = add i32 %y, %x
  ret i32 %tmpadd
}

define i32 @Point3D_constructor(%Point3D* %self, i32 %x, i32 %y, i32 returned %z) {
entry:
  %0 = load i32 (%Point*, i32, i32)*, i32 (%Point*, i32, i32)** getelementptr inbounds (%Point_vTable, %Point_vTable* @Point_vTable, i64 0, i32 1), align 8
  %1 = bitcast %Point3D* %self to %Point*
  %2 = tail call i32 %0(%Point* %1, i32 %x, i32 %y)
  %pz = getelementptr inbounds %Point3D, %Point3D* %self, i64 0, i32 3
  store i32 %z, i32* %pz, align 4
  ret i32 %z
}

define i32 @Point3D_calc(%Point3D* %self) {
entry:
  %0 = load i32 (%Point*)*, i32 (%Point*)** getelementptr inbounds (%Point_vTable, %Point_vTable* @Point_vTable, i64 0, i32 0), align 8
  %1 = bitcast %Point3D* %self to %Point*
  %2 = tail call i32 %0(%Point* %1)
  %pz = getelementptr inbounds %Point3D, %Point3D* %self, i64 0, i32 3
  %z = load i32, i32* %pz, align 4
  %tmpadd = add i32 %z, %2
  ret i32 %tmpadd
}

define i32 @check(%Point* %obj) local_unnamed_addr {
entry:
  %0 = getelementptr inbounds %Point, %Point* %obj, i64 0, i32 0
  %vt = load %Point_vTable*, %Point_vTable** %0, align 8
  %1 = getelementptr inbounds %Point_vTable, %Point_vTable* %vt, i64 0, i32 0
  %2 = load i32 (%Point*)*, i32 (%Point*)** %1, align 8
  %3 = tail call i32 %2(%Point* %obj)
  ret i32 %3
}

attributes #0 = { nofree nounwind }
attributes #1 = { inaccessiblememonly mustprogress nofree nounwind willreturn }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn writeonly }
attributes #3 = { mustprogress nofree norecurse nosync nounwind readonly willreturn }
```



`
